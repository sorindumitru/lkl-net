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

GtkWindow *window;
GtkTreeView *device_list;
//GooCanvasItem *topology;
/*GtkEntry *command;
GtkEntryCompletion *command_completion;
GtkScrolledWindow *output_window;
GtkTextBuffer *output_buffer;
GtkTextView *output;*/

//Toolbar
GtkToolbar *toolbar;
GtkToolButton *boot;

gint callback_boot(GtkWidget *widget, GdkEvent  *event, gpointer   callback_data ) {
	struct params params;
	do_boot_up(&params);
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

	//Toolbar
	toolbar = (GtkToolbar *)gtk_toolbar_new();
	gtk_box_pack_start(GTK_CONTAINER(root), GTK_WIDGET(toolbar), FALSE, FALSE, 10);
	boot = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
	gtk_signal_connect(GTK_WIDGET(boot), "clicked", G_CALLBACK(callback_boot), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), boot, 0);
	
	/*// Device list
	device_list = gtk_tree_view_new();
	// Input box
	command = gtk_entry_new();
	command_completion = gtk_entry_completion_new();
	gtk_widget_show(command);
	//output
	output_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_insert_at_cursor(output_buffer, "A", 1);
	output = gtk_text_view_new_with_buffer(output_buffer);
	gtk_widget_show(output);
	gtk_text_view_set_editable(output, FALSE);
	// Scroll window for output 
	output_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(output_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_widget_show(output_window);
	gtk_scrolled_window_add_with_viewport(output_window, GTK_WIDGET(output));
	gtk_container_set_border_width(GTK_CONTAINER(output_window), 10);
	gtk_container_add(com_and_top, output_window);
	gtk_container_add(com_and_top, command);
	gtk_container_add(root, com_and_top);
	gtk_container_add(window, root);

	gtk_widget_show(root);
	gtk_widget_show(com_and_top);*/

	gtk_container_add(GTK_CONTAINER(window), root);
	return 0;
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
	}
	
	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_window_maximize(window);
	init_gui();
	gtk_widget_show_all(window);
	gtk_main();
	
	return 0;
}
