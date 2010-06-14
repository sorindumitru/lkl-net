#include <gtk/gtk.h>

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
#include <gtktopology.h>

GtkWidget *window;

//Topology;
GtkWidget *topology;

//Device List
GtkWidget *device_list;
GtkListStore *device_store;

//Toolbar
GtkWidget *toolbar;
GtkToolItem *boot;
GtkToolItem *create_router;
GtkToolItem *create_switch;
GtkToolItem *create_hub;
GtkToolItem *create_bridge;

//labels
GtkWidget *ok_label;
GtkWidget *cancel_label;
GtkWidget *create_router_label;
GtkWidget *create_switch_label;

//icons
GtkWidget *router_icon;
GtkWidget *switch_icon;
GtkWidget *hub_icon;
GtkWidget *bridge_icon;

typedef struct dev_entries {
	unsigned char type;
	GtkWidget *dialog;
	GtkWidget *name;
	GtkWidget *config;
	unsigned short port;
} dev_entries_t;

enum
{
  LIST_ITEM = 0,
  N_COLUMNS
};


/// Forward declarations
static void add_device(GtkWidget *list, const gchar *str);
gint timeout_boot(gpointer data);
int init_gui();
void init_labels();
void init_canvas(GtkWidget *box);
void init_device_list(GtkWidget *box);

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

void callback_load(GtkWidget *widget, gpointer   callback_data)
{
	GtkWidget *dialog;
	gchar *filename;

	dialog = gtk_file_chooser_dialog_new("Load config", GTK_WINDOW(window),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					     NULL);

	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		printf("Selected file: %s\n", filename);
	}
	gtk_widget_destroy (dialog);
}

void callback_save(GtkWidget *widget, gpointer   callback_data)
{
	GtkWidget *dialog;
	gchar *filename;

	dialog = gtk_file_chooser_dialog_new("Save config", GTK_WINDOW(window),
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					     NULL);

	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		printf("Selected file: %s\n", filename);
	}
	gtk_widget_destroy (dialog);
}

void callback_dev_create(GtkWidget *widget, gpointer callback_data )
{
	printf("Creating a device\n");
	dev_entries_t *entry = (dev_entries_t*) callback_data;
	if (entry->type < 10) {
		struct params params;
		params.p[2] = malloc(sizeof(char));
		params.p[0] = (char *) gtk_entry_get_text(GTK_ENTRY(entry->name));
		params.p[1] = (char *) gtk_entry_get_text(GTK_ENTRY(entry->config));
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
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_SWITCH);

#if 0
	GtkWidget *dialog = gtk_dialog_new();
	GtkWidget *name_label = gtk_label_new("Hostname:");
	GtkWidget *config_label = gtk_label_new("Config file:");
	GtkWidget *name = gtk_entry_new();
	GtkWidget *config = gtk_entry_new();
	dev_entries_t *ok_entries = malloc(sizeof(*ok_entries));
	ok_entries->type = 1;
	ok_entries->dialog = dialog;
	ok_entries->name = name;
	ok_entries->config = config;
	dev_entries_t *cancel_entries = malloc(sizeof(*cancel_entries));
	cancel_entries->type = 11;
	cancel_entries->dialog = dialog;
	GtkWidget *ok_button = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(ok_button), "clicked", G_CALLBACK(callback_dev_create),(gpointer) ok_entries);
	GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked", G_CALLBACK(callback_dev_create), (gpointer) cancel_entries);
	
	GtkWidget *name_box = gtk_hbox_new(FALSE, 10);
	GtkWidget *config_box = gtk_hbox_new(FALSE,10);
	GtkWidget *buttons = gtk_hbox_new(FALSE, 10);

	
	gtk_container_add(GTK_CONTAINER(name_box), name_label);
	gtk_container_add(GTK_CONTAINER(name_box), name);
	gtk_container_add(GTK_CONTAINER(config_box), config_label);
	gtk_container_add(GTK_CONTAINER(config_box), config);
	gtk_container_add(GTK_CONTAINER(buttons), ok_button);
	gtk_container_add(GTK_CONTAINER(buttons), cancel_button);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), name_box);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), config_box);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), buttons);
	gtk_window_set_title(GTK_WINDOW(dialog), "Create switch");
	gtk_widget_show_all(GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	gtk_dialog_run(GTK_DIALOG(dialog));
#endif
}

void callback_create_router(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_ROUTER);

#if 0
	GtkWidget *dialog = gtk_dialog_new();
	GtkWidget *name_label = gtk_label_new("Hostname:");
	GtkWidget *config_label = gtk_label_new("Config file:");
	GtkWidget *name = gtk_entry_new();
	GtkWidget *config = gtk_entry_new();
	dev_entries_t *ok_entries = malloc(sizeof(*ok_entries));
	ok_entries->type = 0;
	ok_entries->dialog = dialog;
	ok_entries->name = name;
	ok_entries->config = config;
	dev_entries_t *cancel_entries = malloc(sizeof(*cancel_entries));
	cancel_entries->type = 11;
	cancel_entries->dialog = dialog;
	GtkWidget *ok_button = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(ok_button), "clicked", G_CALLBACK(callback_dev_create),(gpointer) ok_entries);
	GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked", G_CALLBACK(callback_dev_create), (gpointer) cancel_entries);
	
	GtkWidget *name_box = gtk_hbox_new(FALSE, 10);
	GtkWidget *config_box = gtk_hbox_new(FALSE,10);
	GtkWidget *buttons = gtk_hbox_new(FALSE, 10);

	
	gtk_container_add(GTK_CONTAINER(name_box), name_label);
	gtk_container_add(GTK_CONTAINER(name_box), name);
	gtk_container_add(GTK_CONTAINER(config_box), config_label);
	gtk_container_add(GTK_CONTAINER(config_box), config);
	gtk_container_add(GTK_CONTAINER(buttons), ok_button);
	gtk_container_add(GTK_CONTAINER(buttons), cancel_button);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), name_box);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), config_box);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), buttons);
	gtk_window_set_title(GTK_WINDOW(dialog), "Create router");
	gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
	gtk_dialog_run(GTK_DIALOG(dialog));
#endif
}

void callback_create_hub(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_HUB);
}

void callback_create_bridge(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_BRIDGE);
}

conf_info_t *info;
hypervisor_t *hypervisor;
pthread_t request;
char *prompt = ">";
unsigned int port;

int init_gui()
{
	GtkWidget *table = gtk_table_new(20, 12, TRUE);
	GtkWidget *topology = gtk_hbox_new(FALSE, 10);
	GtkWidget *load_button;
	GtkWidget *save_button;

	//Toolbar
	toolbar = gtk_toolbar_new();
	// Boot button
	boot = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
	gtk_signal_connect(GTK_OBJECT(boot), "clicked", G_CALLBACK(callback_boot), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), boot, -1);
	// Load config
	load_button = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_signal_connect(GTK_OBJECT(load_button), "clicked", G_CALLBACK(callback_load), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), load_button, -1);
	// Save config
	save_button = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_signal_connect(GTK_OBJECT(save_button), "clicked", G_CALLBACK(callback_save), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save_button, -1);
	// Router button
	create_router = gtk_tool_button_new(router_icon, "New router");
	router_icon = gtk_image_new_from_file("data/icons/router.png");
	gtk_signal_connect(GTK_OBJECT(create_router), "clicked", G_CALLBACK(callback_create_router), NULL);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(create_router), router_icon);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(create_router), "New router");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), create_router, -11);
	// Switch button
	switch_icon = gtk_image_new_from_file("data/icons/switch.png");
	create_switch = gtk_tool_button_new(switch_icon, "New switch");
	gtk_signal_connect(GTK_OBJECT(create_switch), "clicked", G_CALLBACK(callback_create_switch), NULL);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(create_switch), switch_icon);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(create_switch), "New switch");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), create_switch, -1);
	// Hub button
	create_hub = gtk_tool_button_new(hub_icon, "New hub");
	hub_icon = gtk_image_new_from_file("data/icons/hub.png");
	gtk_signal_connect(GTK_OBJECT(create_hub), "clicked", G_CALLBACK(callback_create_hub), NULL);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(create_hub), hub_icon);
	gtk_tool_button_set_label(create_hub, "New hub");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), create_hub, -1);
	// Bridge button
	create_bridge = gtk_tool_button_new(bridge_icon, "New Bridge");
	bridge_icon = gtk_image_new_from_file("data/icons/bridge.png");
	gtk_signal_connect(GTK_OBJECT(create_bridge), "clicked", G_CALLBACK(callback_create_bridge), NULL);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(create_bridge), bridge_icon);
	gtk_tool_button_set_label(create_bridge, "New bridge");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), create_bridge, -1);
	

	init_device_list(topology);
	init_canvas(topology);

	gtk_table_attach(GTK_TABLE(table), toolbar, 0, 12, 0, 2, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), topology, 2, 10, 2, 20, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), device_list, 10, 12, 2, 20, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_container_add(GTK_CONTAINER(window), table);
	return 0;
}

void init_labels()
{
	ok_label = gtk_label_new("Ok");
	cancel_label = gtk_label_new("Cancel");
	create_router_label = gtk_label_new("Create router");
	create_switch_label = gtk_label_new("Create switch");
}

void on_changed(GtkWidget *widget, gpointer label) 
{

}

void init_canvas(GtkWidget *box)
{
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
        topology = gtk_topology_new();
	gtk_widget_set_size_request(topology,2560,2048);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), topology);
	gtk_container_add(GTK_CONTAINER(box), scrolled_window);
	//gtk_topology_add_device(GTK_TOPOLOGY(topology), gtk_topology_new_router());
}

void init_device_list(GtkWidget *box)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	device_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(device_list), TRUE);

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
	gtk_window_maximize(GTK_WINDOW(window));
	init_gui();

	list_for_each_safe(head, temp, &info->devices){
		device_t *dev = list_entry(head, device_t, list);
		GtkTopologyDevice *device;
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
			device = gtk_topology_new_router(dev);
			gtk_topology_add_device(GTK_TOPOLOGY(topology), device);
			break;
		default:
			break;
		}
		add_device(device_list, dev->hostname);
	}

	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), G_OBJECT(window));

	gtk_widget_set_events(window, GDK_BUTTON_RELEASE_MASK);
	
	gtk_widget_show_all(window);
	gtk_main();
	
	return 0;
}
