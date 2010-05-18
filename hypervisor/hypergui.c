
#include <gtk/gtk.h>
#include <goocanvas.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>

#include <config.h>
#include <device.h>
#include <hypervisor.h>
#include <hypervisor_cmd.h>

GtkWindow *window;

/*GtkEntry *command;
GtkEntryCompletion *command_completion;
GtkScrolledWindow *output_window;
GtkTextBuffer *output_buffer;
GtkTextView *output;*/

//Device List
GtkTreeView *device_list;
GtkListStore *device_store;

//Canvas
GooCanvasItem *canvas;

//Toolbar
GtkToolbar *toolbar;
GtkToolButton *boot;
GtkToolButton *create_router;
GtkToolButton *create_switch;

//labels
GtkLabel *ok_label;
GtkLabel *cancel_label;
GtkLabel *create_router_label;
GtkLabel *create_switch_label;

typedef struct dev_entries {
	unsigned char type;
	GtkDialog *dialog;
	GtkEntry *name;
	GtkEntry *config;
	unsigned short port;
} dev_entries_t;

enum
{
  LIST_ITEM = 0,
  N_COLUMNS
};

static void add_device(GtkWidget *list, const gchar *str)
{
	GtkTreeIter iter;
	
	gtk_list_store_append(device_store, &iter);
	gtk_list_store_set(device_store, &iter, LIST_ITEM, str, -1);
}

gint timeout_boot( gpointer data )
{
	struct params params;
	do_boot_up(&params);
	return FALSE;
}

void callback_boot(GtkWidget *widget, gpointer   callback_data)
{
	gtk_timeout_add(1000, timeout_boot, NULL);
}

void callback_dev_create(GtkWidget *widget, gpointer callback_data )
{
	printf("Creating a device\n");
	dev_entries_t *entry = (dev_entries_t*) callback_data;
	if (entry->type < 10) {
		struct params params;
		params.p[2] = malloc(sizeof(char));
		params.p[0] = (char *) gtk_entry_get_text(entry->name);
		params.p[1] = (char *) gtk_entry_get_text(entry->config);
		printf("Starting device\n");
		if( entry->type == 0){
			do_create_router(&params);
		} else {
			do_create_switch(&params);
		}
	}
	gtk_widget_destroy(GTK_WIDGET(entry->dialog));
}

void callback_create_switch(GtkWidget *widget, gpointer   callback_data )
{
	GtkDialog *dialog = gtk_dialog_new();
	GtkLabel *name_label = (GtkLabel*) gtk_label_new("Hostname:");
	GtkLabel *config_label = (GtkLabel*) gtk_label_new("Config file:");
	GtkEntry *name = gtk_entry_new();
	GtkEntry *config = gtk_entry_new();
	dev_entries_t *ok_entries = malloc(sizeof(*ok_entries));
	ok_entries->type = 1;
	ok_entries->dialog = dialog;
	ok_entries->name = name;
	ok_entries->config = config;
	dev_entries_t *cancel_entries = malloc(sizeof(*cancel_entries));
	cancel_entries->type = 11;
	cancel_entries->dialog = dialog;
	GtkButton *ok_button = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_WIDGET(ok_button), "clicked", G_CALLBACK(callback_dev_create),(gpointer) ok_entries);
	GtkButton *cancel_button = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_WIDGET(cancel_button), "clicked", G_CALLBACK(callback_dev_create), (gpointer) cancel_entries);
	
	GtkHBox *name_box = gtk_hbox_new(FALSE, 10);
	GtkHBox *config_box = gtk_hbox_new(FALSE,10);
	GtkHBox *buttons = gtk_hbox_new(FALSE, 10);

	
	gtk_container_add(name_box, name_label);
	gtk_container_add(name_box, name);
	gtk_container_add(config_box, config_label);
	gtk_container_add(config_box, config);
	gtk_container_add(buttons, ok_button);
	gtk_container_add(buttons, cancel_button);
	gtk_container_add(dialog->vbox, name_box);
	gtk_container_add(dialog->vbox, config_box);
	gtk_container_add(dialog->vbox, buttons);
	gtk_window_set_title(dialog, "Create switch");
	gtk_widget_show_all(dialog->vbox);
	gtk_dialog_run(dialog);
}

void callback_create_router(GtkWidget *widget, gpointer   callback_data )
{
	GtkDialog *dialog = gtk_dialog_new();
	GtkLabel *name_label = gtk_label_new("Hostname:");
	GtkLabel *config_label = gtk_label_new("Config file:");
	GtkEntry *name = gtk_entry_new();
	GtkEntry *config = gtk_entry_new();
	dev_entries_t *ok_entries = malloc(sizeof(*ok_entries));
	ok_entries->type = 0;
	ok_entries->dialog = dialog;
	ok_entries->name = name;
	ok_entries->config = config;
	dev_entries_t *cancel_entries = malloc(sizeof(*cancel_entries));
	cancel_entries->type = 11;
	cancel_entries->dialog = dialog;
	GtkButton *ok_button = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_WIDGET(ok_button), "clicked", G_CALLBACK(callback_dev_create),(gpointer) ok_entries);
	GtkButton *cancel_button = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_WIDGET(cancel_button), "clicked", G_CALLBACK(callback_dev_create), (gpointer) cancel_entries);
	
	GtkHBox *name_box = gtk_hbox_new(FALSE, 10);
	GtkHBox *config_box = gtk_hbox_new(FALSE,10);
	GtkHBox *buttons = gtk_hbox_new(FALSE, 10);

	
	gtk_container_add(name_box, name_label);
	gtk_container_add(name_box, name);
	gtk_container_add(config_box, config_label);
	gtk_container_add(config_box, config);
	gtk_container_add(buttons, ok_button);
	gtk_container_add(buttons, cancel_button);
	gtk_container_add(dialog->vbox, name_box);
	gtk_container_add(dialog->vbox, config_box);
	gtk_container_add(dialog->vbox, buttons);
	gtk_window_set_title(dialog, "Create router");
	gtk_widget_show_all(dialog->vbox);
	gtk_dialog_run(dialog);
}

conf_info_t *info;
hypervisor_t *hypervisor;
pthread_t request;
char *prompt = ">";
unsigned int port;

int init_gui()
{
	GtkVBox *root = (GtkVBox*) gtk_vbox_new(FALSE, 10);
	GtkHBox *topology = (GtkHBox *)gtk_hbox_new(FALSE, 10);
	GtkWidget *image;
	image = gtk_image_new_from_file ("web.png");

	//Toolbar
	toolbar = (GtkToolbar *)gtk_toolbar_new();
	//gtk_box_pack_start(GTK_CONTAINER(root), GTK_WIDGET(toolbar), FALSE, FALSE, 10);
	// Boot button
	boot = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
	gtk_signal_connect(G_OBJECT(boot), "clicked", G_CALLBACK(callback_boot), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), boot, 0);
	// Router button
	create_router = gtk_tool_button_new(create_router_label, "New router");
	gtk_signal_connect(create_router, "clicked", G_CALLBACK(callback_create_router), NULL);
	gtk_tool_button_set_icon_widget(create_router, create_router_label);
	gtk_tool_button_set_label(create_router, "New router");
	gtk_toolbar_insert(toolbar, create_router, 1);
	// Switch button
	create_switch = gtk_tool_button_new(create_switch_label, "New switch");
	gtk_signal_connect(create_switch, "clicked", G_CALLBACK(callback_create_switch), NULL);
	gtk_tool_button_set_icon_widget(create_switch, create_switch_label);
	gtk_tool_button_set_label(create_switch, "New switch");
	gtk_toolbar_insert(toolbar, create_switch, 2);

	init_device_list(topology);
	//init_canvas(topology);
	gtk_container_add (GTK_CONTAINER (topology), image);

	
	gtk_container_add(GTK_CONTAINER(root), GTK_WIDGET(toolbar));
	gtk_container_add(GTK_CONTAINER(root), GTK_WIDGET(topology));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(root));
	return 0;
}

void init_labels()
{
	ok_label = (GtkLabel*) gtk_label_new("Ok");
	cancel_label = (GtkLabel*) gtk_label_new("Cancel");
	create_router_label = gtk_label_new("Create router");
	create_switch_label = gtk_label_new("Create switch");
}

void on_changed(GtkWidget *widget, gpointer label) 
{

}

void init_canvas(GtkHBox *box)
{
        canvas = goo_canvas_new();
	gtk_widget_set_size_request(canvas, 800, 600);
	goo_canvas_set_bounds(GOO_CANVAS (canvas), 0, 0, 800, 600);
	gtk_widget_show(canvas);
	gtk_container_add(GTK_CONTAINER(box), canvas);
}

void init_device_list(GtkHBox *box)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	device_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(device_list), TRUE);
	gtk_box_pack_start(GTK_BOX(box), device_list, TRUE, TRUE, 10);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Devices", renderer, "text", LIST_ITEM, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(device_list), column);
	device_store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(device_list), GTK_TREE_MODEL(device_store));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(device_list));

	g_signal_connect(selection, "changed", G_CALLBACK(on_changed), NULL);
}

int main(int argc, char **argv)
{
	struct list_head *head, *temp;
	char *command;
	info = malloc(sizeof(*info));
	hypervisor = malloc(sizeof(*hypervisor));
	config_init(info);
	config_read_file(info, argv[1]);
	init_hypervisor(hypervisor,info);
	
	printf("LKL NET :: Hypervisor is starting\n");

	start_request_thread();
	initialize_autocomplete();
	port = info->general.port;

	gtk_set_locale();
	gtk_init(&argc, &argv);

	init_labels();
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_window_maximize(window);
	init_gui();

	list_for_each_safe(head, temp, &info->devices){
		device_t *dev = list_entry(head, device_t, list);
		list_del(head);
		INIT_LIST_HEAD(&dev->list);
		switch(dev->type){
		case DEV_HUB:
			list_add(&dev->list, &hypervisor->links);
			break;
		case DEV_SWITCH:
			list_add(&dev->list, &hypervisor->switches);
			break;
		case DEV_ROUTER:
			list_add(&dev->list, &hypervisor->routers);
			break;
		default:
			break;
		}
		add_device(device_list, dev->hostname);
	}

	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), G_OBJECT(window));

	gtk_widget_set_events(GTK_WIDGET(window), GDK_BUTTON_RELEASE_MASK);
	
	gtk_widget_show_all(window);
	gtk_main();
	
	return 0;
}
