#include <gio/gio.h>
#include <gtksourceview/gtksourceview.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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

// Interface List
GtkWidget *interface_list;
GtkListStore *interface_store;

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
  DEV_NAME = 0,
  DEV_N_COLUMNS
};

enum {
	IF_NAME = 0,
	IF_LINK,
	IF_IP,
	IF_NETMASK,
	IF_MAC,
	IF_N_COLUMNS,
};

conf_info_t *info;
hypervisor_t *hypervisor;
pthread_t request;
char *prompt = ">";
unsigned int port;
char *confdir;
unsigned int deviceport = 55000;

/// Forward declarations
static void add_device(GtkWidget *list, const gchar *str);
gint timeout_boot(gpointer data);
int init_gui();
void init_labels();
void init_canvas(GtkWidget *box);
void init_device_list(GtkWidget *box);
void init_topology_devices();

// Callbacks
void on_changed(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column,  gpointer user_data);
void callback_if_select(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column,  gpointer user_data);

// Dialogs
void router_dialog(GtkTopologyDevice *device, GdkWindow *window);
void error_dialog(const gchar *message);

static void add_device(GtkWidget *list, const gchar *str)
{
	GtkTreeIter iter;
	
	gtk_list_store_append(device_store, &iter);
	gtk_list_store_set(device_store, &iter, DEV_NAME, str, -1);
}

void init_topology_devices()
{
	struct list_head *head, *temp;
	list_for_each_safe(head, temp, &info->devices){
		conf_info_t *dev_info = malloc(sizeof(*dev_info));
		device_t *dev = list_entry(head, device_t, list);
		GtkTopologyDevice *device = NULL;
		struct list_head *ihead;
		list_del(head);
		INIT_LIST_HEAD(&dev->list);
		switch(dev->type){
		case DEV_HUB:
			list_add(&dev->list, &hypervisor->links);
			device = gtk_topology_new_hub(dev);
			break;
		case DEV_SWITCH:
			list_add(&dev->list, &hypervisor->switches);
			device = gtk_topology_new_switch(dev);
			break;
		case DEV_ROUTER:
			list_add(&dev->list, &hypervisor->routers);
			device = gtk_topology_new_router(dev);
			device->dialog = router_dialog;
			break;
		default:
			break;
		}
		INIT_LIST_HEAD(&dev->interfaces);
		if (dev->type != DEV_HUB) {
			config_init(dev_info);
			config_read_file(dev_info, dev->config);
                        dev->port = dev_info->general.port;
			list_for_each(ihead, &dev_info->interfaces) {
				interface_t *intf = list_entry(ihead, interface_t, list);
				interface_t *dev_if = malloc(sizeof(*dev_if));
				memcpy(dev_if, intf, sizeof(*intf));
				INIT_LIST_HEAD(&dev_if->list);
				list_add(&dev_if->list, &dev->interfaces); 
			}
		}
		gtk_topology_add_device(GTK_TOPOLOGY(topology), device);
		add_device(device_list, dev->hostname);
	}

	gtk_topology_add_links(GTK_TOPOLOGY(topology));
	gtk_widget_queue_draw(topology);
}

gint timeout_boot( gpointer data )
{
	struct params params;
	do_boot_up(&params);
	return FALSE;
}

gint timeout_kill( gpointer data )
{
	struct params params;
	do_kill_all(&params);
	return FALSE;
}

gint timeout_load(gpointer data)
{
	gchar *filename = (gchar *) data;
	if (info->read) 
		config_free(info);
	
	info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, filename);
	init_hypervisor(hypervisor,info);

	init_topology_devices();
	
	return FALSE;
}

gint timeout_save(gpointer data)
{
	gchar *filename = (gchar *) data;
	struct list_head *head;
	GtkTopology *top = GTK_TOPOLOGY(topology);
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		error_dialog("Could not open file");
		return FALSE;
	}
	
	if (write(fd, "hypervisor {\n", strlen("hypervisor {\n")) < 0) {
		error_dialog("Could not write file");
		return FALSE;
	}
	
	list_for_each(head, &top->devices) {
		GtkTopologyDevice *device = list_entry(head, GtkTopologyDevice, list);
		char buffer[256];
		if (write(fd, "\tdevice {\n", strlen("\tdevice {\n")) < 0) {
			error_dialog("Could not write file");
			return FALSE;
		}
		memset(buffer, 0, 256);
		sprintf(buffer, "\t\ttype %s;\n", get_type(device->dev->type));
		if (write(fd, buffer, strlen(buffer)) < 0) {
			error_dialog("Could not write file");
			return FALSE;
		}
		memset(buffer, 0, 256);
		sprintf(buffer,"\t\thostname %s;\n",device->dev->hostname);
		if (write(fd, buffer, strlen(buffer)) < 0) {
			error_dialog("Could not write file");
			return FALSE;
		}
		if (device->dev->type != DEV_HUB) {
			memset(buffer,0,128);
			sprintf(buffer,"\t\tconfig %s;\n",device->dev->config);
			if (write(fd, buffer, strlen(buffer)) < 0) {
				error_dialog("Could not write file");
			}
		} else {
			memset(buffer,0,128);
			sprintf(buffer,"\t\tport %d;\n", device->dev->port);
			if (write(fd, buffer, strlen(buffer)) < 0) {
				error_dialog("Could not write file");
			}
		}
		memset(buffer, 0, 256);
		sprintf(buffer,"\t\tposx %d;\n\t\tposy %d\n",device->dev->x, device->dev->y);
		if (write(fd, buffer, strlen(buffer)) < 0) {
			error_dialog("Could not write file");
			return FALSE;
		}
		if (write(fd, "\t}\n", strlen("\t}\n")) < 0) {
			error_dialog("Could not write file");
			return FALSE;
		}
	}
	
	if (write(fd, "}\n", strlen("}\n")) < 0) {
		error_dialog("Could not write file");
		return FALSE;
	}
	
	close(fd);
	return FALSE;
}

char data;

void callback_verify_ok(GtkWidget *widget, gpointer   callback_data) 
{
	GtkWidget *dialog = (GtkWidget *) callback_data;
	data = 1;
	gtk_widget_hide_all(dialog);
}

void callback_verify_cancel(GtkWidget *widget, gpointer   callback_data) 
{
	GtkWidget *dialog = (GtkWidget *) callback_data;
	gtk_widget_hide_all(dialog);
}

void error_dialog(const gchar *message) {

}

struct add_if {
        interface_t* interface;
        device_t* dev;
};

gint notify_add_if(gpointer data)
{	
        interface_t *interface = ((struct add_if*) data)->interface;
        device_t *dev = ((struct add_if*) data)->dev;
        request_t *request = malloc(sizeof(*request) + sizeof(interface_t));
        socket_t *socketp;
        struct hostent *host;
        struct sockaddr_in device;
        int err, sock;

        request->length = sizeof(*request) + sizeof(request_add_if_t);
        request->type = REQ_ADD_IF;
        //memcpy(request->data, interface, sizeof(*interface));

        socketp = get_device_socket(dev);
        host = gethostbyname(socketp->address);
        device.sin_family = AF_INET;
        device.sin_port = socketp->port;
        device.sin_addr = *(struct in_addr*) host->h_addr;
        free(socketp);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
                perror("could not open socket");
        }
        err = connect(sock, (struct sockaddr*) &device, sizeof(device));
        if (err < 0) {
                perror("could not connect");
        }

        err = send(sock, request, sizeof(*request),  0);
        if (err < 0) {
                perror("could not send data");
        }
        err = send(sock, interface, sizeof(*interface),  0);
        if (err < 0) {
                perror("could not send data");
        }
        int size = strlen(interface->dev)+1;
        err = send(sock, &size, sizeof(size),  0);
        if (err < 0) {
                perror("could not send data");
        }
        err = send(sock, interface->dev, strlen(interface->dev)+1,  0);
        if (err < 0) {
                perror("could not send data");
        }
        if (interface->link) {
                size = strlen(interface->link)+1;
                err = send(sock, &size, sizeof(size),  0);
                if (err < 0) {
                        perror("could not send data");
                }
                err = send(sock, interface->link, strlen(interface->link)+1,  0);
                if (err < 0) {
                        perror("could not send data");
                }
        } else { 
                size = -1;
                err = send(sock, &size, sizeof(size),  0);
                if (err < 0) {
                        perror("could not send data");
                }
        }
        err = send(sock, interface->mac, sizeof(*(interface->mac)),  0);
        if (err < 0) {
                perror("could not send data");
        }
        
	return FALSE;
}

void callback_add_if(GtkWidget *widget, gpointer   callback_data)
{
	GtkTopologyDevice *device = (GtkTopologyDevice*) callback_data;
	GtkWidget *dialog = gtk_dialog_new();
	GtkWidget *table = gtk_table_new(12, 12, FALSE);
	GtkWidget *name_label, *name_entry;
	GtkWidget *ip_label, *ip_entry;
	GtkWidget *mac_label, *mac_entry;
	GtkWidget *ok, *cancel;

	name_label = gtk_label_new("Name");
	name_entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), name_label, 0, 6, 0, 2, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), name_entry, 6, 12, 0, 2, GTK_FILL, GTK_SHRINK, 0, 0);

	ip_label = gtk_label_new("Ip");
	ip_entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), ip_label, 0, 6, 4, 6, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), ip_entry, 6, 12, 4, 6, GTK_FILL, GTK_SHRINK, 0, 0);

	mac_label = gtk_label_new("Mac");
	mac_entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), mac_label, 0, 6, 6, 8, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), mac_entry, 6, 12, 6, 8, GTK_FILL, GTK_SHRINK, 0, 0);

	ok = gtk_button_new_with_label("Ok");
	cancel = gtk_button_new_with_label("Cancel");
	gtk_table_attach(GTK_TABLE(table), ok, 0, 6, 8, 10, GTK_FILL, GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", G_CALLBACK(callback_verify_ok), dialog);
	gtk_table_attach(GTK_TABLE(table), cancel, 6, 12, 8, 10, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked", G_CALLBACK(callback_verify_cancel), dialog);
	
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), table);
	data = 0;
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	
	//validate entrys;
	if (data == 1) {
		GtkTreeIter iter;
		interface_t *interface = malloc(sizeof(*interface));
		char *ip_data = strtok((char *)gtk_entry_get_text(GTK_ENTRY(ip_entry)), "/");
		char *null = "";
		if (!ip_data) {
			error_dialog("Invalid address");
			return;
		}
		char *netmask = strtok(NULL, "/");
		if (!netmask) {
			error_dialog("Invalid address");
			return;
		}
		interface->dev = strdup(gtk_entry_get_text(GTK_ENTRY(name_entry)));
		interface->link = NULL;
		if (!inet_pton(AF_INET, gtk_entry_get_text(GTK_ENTRY(ip_entry)), &interface->address)) {
			
		}
		interface->netmask_len = atoi(netmask);
		interface->mac = ether_aton(gtk_entry_get_text(GTK_ENTRY(mac_entry)));
		if (!interface->mac) {
			error_dialog("Invalid address");
			return;
		}
		
		INIT_LIST_HEAD(&interface->list);
		list_add(&interface->list, &device->dev->interfaces);
		
		gtk_list_store_append(interface_store, &iter);
		gtk_list_store_set(interface_store, &iter,
				   IF_NAME, interface->dev,
				   IF_LINK, interface->link != NULL ? interface->link : null,
				   IF_IP, ip_data,
				   IF_NETMASK, interface->netmask_len,
				   IF_MAC, ether_ntoa(interface->mac),
				   -1
				   );
                struct add_if *ai= malloc(sizeof(*ai));
                ai->interface = interface;
                ai->dev = device->dev;
                gtk_timeout_add(500, notify_add_if, ai);
	}
	gtk_widget_destroy(dialog);
}

void callback_remove_if(GtkWidget *widget, gpointer   callback_data)
{
	struct list_head *head, *temp;
	char *interface_name;
	GtkTopologyDevice *device = (GtkTopologyDevice*) callback_data;
	GtkTreeIter if_iter;
	GtkTreeModel *model = GTK_TREE_MODEL(interface_store);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(interface_list));
	gtk_tree_selection_get_selected(sel, &model, &if_iter);
	gtk_tree_model_get(GTK_TREE_MODEL(interface_store), &if_iter, 0, &interface_name, -1);
	
	list_for_each_safe(head, temp, &device->dev->interfaces) {
		interface_t *interface = list_entry(head, interface_t, list);
		if (!strcmp(interface_name, interface->dev)) {
			gtk_topology_del_interface_link(GTK_TOPOLOGY(topology), interface);
			list_del(head);
			free(interface);
		}
	}
	
	gtk_list_store_remove(interface_store, &if_iter);
}

void callback_if_select(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column,  gpointer user_data)
{
	GtkTreeIter if_iter;
	char *ifname;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(interface_store), &if_iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(interface_store), &if_iter, 0, &ifname, -1);
}

void router_dialog(GtkTopologyDevice *device, GdkWindow *window)
{
	GtkWidget *dialog = gtk_dialog_new();
	GtkWidget *add = gtk_button_new_with_label("Add");
	GtkWidget *remove = gtk_button_new_with_label("Remove");
	GtkWidget *table = gtk_table_new(12, 12, FALSE);
	GtkCellRenderer *renderer;
	char *null = "";
	GtkTreeViewColumn *name, *link, *ip, *mac, *netmask;
	char ip_data[32];
	GtkTreeIter iter;
	struct list_head *head;

	interface_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(interface_list), TRUE);
	renderer = gtk_cell_renderer_text_new();
	name = gtk_tree_view_column_new_with_attributes("Interface", renderer, "text", IF_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(interface_list), name);
	link = gtk_tree_view_column_new_with_attributes("Link", renderer, "text", IF_LINK, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(interface_list), link);
	ip = gtk_tree_view_column_new_with_attributes("Ip address", renderer, "text", IF_IP, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(interface_list), ip);
	netmask = gtk_tree_view_column_new_with_attributes("Netmask", renderer, "text", IF_NETMASK, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(interface_list), netmask);
	mac = gtk_tree_view_column_new_with_attributes("Mac address", renderer, "text", IF_MAC, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(interface_list), mac);
	interface_store = gtk_list_store_new(IF_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(interface_list), GTK_TREE_MODEL(interface_store));

	gtk_table_attach(GTK_TABLE(table), interface_list, 0, 12, 0, 10, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), add, 0, 6, 10, 12, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_signal_connect(GTK_OBJECT(add), "clicked", G_CALLBACK(callback_add_if), device);
	gtk_table_attach(GTK_TABLE(table), remove, 6, 12, 10, 12, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_signal_connect(GTK_OBJECT(remove), "clicked", G_CALLBACK(callback_remove_if), device);

	list_for_each(head, &device->dev->interfaces) {
		interface_t *interface = list_entry(head, interface_t, list);
		gtk_list_store_append(interface_store, &iter);
		inet_ntop(AF_INET, &interface->address.s_addr, ip_data, 32);
                char *mac = ether_ntoa(interface->mac);
                printf("%s\n", mac);
		gtk_list_store_set(interface_store, &iter,
				   IF_NAME, interface->dev,
				   IF_LINK, interface->link != NULL ? interface->link : null,
				   IF_IP, ip_data,
				   IF_NETMASK, interface->netmask_len,
				   IF_MAC, mac,
				   -1
				   );
	}

	g_signal_connect(GTK_OBJECT(interface_list), "row-activated", G_CALLBACK(callback_if_select), NULL);
	
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), table);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	
}

void callback_boot(GtkWidget *widget, gpointer   callback_data)
{
	gtk_timeout_add(1000, timeout_boot, NULL);
}

void callback_kill(GtkWidget *widget, gpointer   callback_data)
{
		gtk_timeout_add(1000, timeout_kill, NULL);
}

void callback_cancel(GtkWidget *widget, gpointer   callback_data)
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_NONE);
}

void callback_delete(GtkWidget *widget, gpointer   callback_data)
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_DEL_DEVICE);
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
		gtk_timeout_add(500, timeout_load, filename);
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
		gtk_timeout_add(250, timeout_save, filename);
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
}

void callback_create_router(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_ROUTER);
}

void callback_create_hub(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_HUB);
}

void callback_create_bridge(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_BRIDGE);
}

void callback_add_link(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_ADD_LINK);
}

void callback_del_link(GtkWidget *widget, gpointer   callback_data )
{
	gtk_topology_set_selection(GTK_TOPOLOGY(topology), SEL_DEL_LINK);
}

int init_gui()
{
	GtkWidget *table = gtk_table_new(20, 12, FALSE);
	GtkWidget *topology = gtk_hbox_new(FALSE, 10);
	GtkToolItem *kill;
	GtkToolItem *load_button;
	GtkToolItem *save_button;
	GtkToolItem *cancel_button;
	GtkToolItem *delete_button;
	GtkToolItem *add_link;
	GtkWidget *add_link_icon;
	GtkToolItem *del_link;
	GtkWidget *del_link_icon;

	//Toolbar
	toolbar = gtk_toolbar_new();
	// Boot button
	boot = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
	gtk_signal_connect(GTK_OBJECT(boot), "clicked", G_CALLBACK(callback_boot), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), boot, -1);
	kill = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_signal_connect(GTK_OBJECT(kill), "clicked", G_CALLBACK(callback_kill), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), kill, -1);

	// Load config
	load_button = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_signal_connect(GTK_OBJECT(load_button), "clicked", G_CALLBACK(callback_load), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), load_button, -1);
	// Save config
	save_button = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_signal_connect(GTK_OBJECT(save_button), "clicked", G_CALLBACK(callback_save), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save_button, -1);

	GtkToolItem *sep = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep, -1);
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
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(create_hub), "New hub");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), create_hub, -1);
	// Bridge button
	create_bridge = gtk_tool_button_new(bridge_icon, "New Bridge");
	bridge_icon = gtk_image_new_from_file("data/icons/bridge.png");
	gtk_signal_connect(GTK_OBJECT(create_bridge), "clicked", G_CALLBACK(callback_create_bridge), NULL);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(create_bridge), bridge_icon);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(create_bridge), "New bridge");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), create_bridge, -1);

	GtkToolItem *sep1 = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep1, -1);
	// Add link button
	add_link_icon = gtk_image_new_from_file("data/icons/add_link.png");
	add_link = gtk_tool_button_new(add_link_icon, "Add link");
	gtk_signal_connect(GTK_OBJECT(add_link), "clicked", G_CALLBACK(callback_add_link), NULL);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(add_link), add_link_icon);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(add_link), "New bridge");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), add_link, -1);
	// Del link button
	del_link_icon = gtk_image_new_from_file("data/icons/del_link.png");
	del_link = gtk_tool_button_new(del_link_icon, "Delete link");
	gtk_signal_connect(GTK_OBJECT(del_link), "clicked", G_CALLBACK(callback_del_link), NULL);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(del_link), del_link_icon);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(del_link), "New bridge");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), del_link, -1);
	
	GtkToolItem *sep2 = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep2, -1);
	// Delete button
	delete_button = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_signal_connect(GTK_OBJECT(delete_button), "clicked", G_CALLBACK(callback_delete), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), delete_button, -1);
	// Cancel button
	cancel_button = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked", G_CALLBACK(callback_cancel), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), cancel_button, -1);
	

	init_device_list(topology);
	init_canvas(topology);
	
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), device_list);

	gtk_table_attach(GTK_TABLE(table), toolbar, 0, 12, 0, 2, GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(table), topology, 2, 10, 2, 20, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), scrolled_window, 10, 12, 2, 20, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
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

void on_changed(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column,  gpointer user_data)
{
	GtkWidget *dialog;
	dialog = gtk_dialog_new();
	GtkWidget *source_view;
	GtkSourceBuffer *source_buffer = gtk_source_buffer_new(NULL);
	gchar *device_name;
	GtkTreeIter iter;
	GtkTopology *top = GTK_TOPOLOGY(topology);
	struct list_head *head;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(device_store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(device_store), &iter, 0, &device_name, -1);

	source_view = gtk_source_view_new_with_buffer(source_buffer);

	list_for_each(head, &top->devices) {
		GtkTopologyDevice *device = list_entry(head, GtkTopologyDevice, list);
		if (!strcmp(device_name, device->dev->hostname) && device->dev->config) {
			GError *error;
			gchar *buffer;
			//open file
			printf("config %s\n", device->dev->config);
			if (!g_file_get_contents (device->dev->config, &buffer, NULL, &error)) {
				return;
			}
			gtk_source_buffer_begin_not_undoable_action(source_buffer);
			gtk_text_buffer_set_text (GTK_TEXT_BUFFER(source_buffer), buffer, -1);
			gtk_source_buffer_end_not_undoable_action(source_buffer);
			gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(source_buffer), FALSE);
			gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(source_view), TRUE);
			gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(source_view), TRUE);
			gtk_widget_set_sensitive (GTK_WIDGET(source_view), FALSE);
			gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), source_view);
			gtk_widget_show_all(dialog);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		}
	}
}

gint notify_device(gpointer data)
{
	GtkTopologyDevice *device = (GtkTopologyDevice*) data;
	
	if (device->list.next == NULL && device->list.prev == NULL) {
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_string("0");
		while(1) {
			char *device_name;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(device_store), &iter, path);
			gtk_tree_model_get(GTK_TREE_MODEL(device_store), &iter, 0, &device_name, -1);
			if (!strcmp(device_name, device->dev->hostname)) {
				gtk_list_store_remove(device_store, &iter);
				return FALSE;
			}
			gtk_tree_path_next(path);
		}
		//GtkTreeModel *model = GTK_TREE_MODEL(device_store);
		//gtk_list_store_remove(device_store, &if_iter);
		list_del(&device->dev->list);
		return FALSE;
	}
	
	add_device(device_list, device->dev->hostname);
	INIT_LIST_HEAD(&device->dev->list);
	device->dialog = router_dialog;
	switch(device->dev->type){
	case DEV_HUB:
		list_add(&device->dev->list, &hypervisor->links);
		break;
	case DEV_BRIDGE:
		break;
	case DEV_ROUTER:
		list_add(&device->dev->list, &hypervisor->routers);
		break;
	case DEV_SWITCH:
		list_add(&device->dev->list, &hypervisor->switches);
		break;
	default:
		break;
	}

        device->dev->port = deviceport;

        if (device->dev->type == DEV_ROUTER || device->dev->type == DEV_SWITCH) {
                printf("%p\n", device->dev->hostname);
                char *conf_file = malloc(255);
                char data[255];
                sprintf(conf_file,"%s/%s", confdir, device->dev->hostname);
                int fd = open(conf_file, O_CREAT|O_TRUNC|O_WRONLY, 0666);
                
                sprintf(data,"hostname %s;\nipaddress 127.0.0.1;\nport %d\n", device->dev->hostname, deviceport++);
                if (write(fd, data, strlen(data)) < 0) {
                        perror("write device data");
                }
                printf("%s %s\n", data, conf_file);
                close(fd);
                device->dev->config = conf_file;
        } else { 
		deviceport++;
	}

	return FALSE;
}

gint notify_link(gpointer data)
{	
        printf("A\n");
        GtkTopologyLink *link = (GtkTopologyLink*) data;
        device_t *hub, *dev;
        request_t *request = malloc(sizeof(*request));
        socket_t *socketd;
        socket_t *socketh;
        struct hostent *host;
        struct sockaddr_in device;
        int err, sock;
        printf("\\n");

        if (link->end1->dev->type == DEV_HUB) {
                hub = link->end1->dev;
                dev = link->end2->dev;
        } else {
                hub = link->end2->dev;
                dev = link->end1->dev;
        }

        request->length = sizeof(*request);
        request->type = REQ_ADD_LINK;

        printf("AAA %p\n",dev);

        socketd = get_device_socket(dev);
        host = gethostbyname(socketd->address);
        device.sin_family = AF_INET;
        device.sin_port = socketd->port;
        device.sin_addr = *(struct in_addr*) host->h_addr;
        free(socketd);

        printf("conn mconfig\n");

        socketh = get_device_socket(hub);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
                perror("could not open socket");
        }
        err = connect(sock, (struct sockaddr*) &device, sizeof(device));
        if (err < 0) {
                perror("could not connect");
        }
        err = send(sock, request, sizeof(*request),  0);
        if (err < 0) {
                perror("could not send data");
        }
        err = send(sock, socketh, sizeof(*socketh), 0);
        if (err < 0) {
                perror("could not send data");
        }
        int size = strlen(link->interface->dev)+1;
        err = send(sock, &size, sizeof(size), 0);
        if (err < 0) {
                perror("could not send data");
        }
        err = send(sock, link->interface->dev, size, 0);
        if (err < 0) {
                perror("could not send data");
        }
        free(socketh);

	return FALSE;
}

void init_canvas(GtkWidget *box)
{
	GtkTopology *top;
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
        topology = gtk_topology_new();
	gtk_widget_set_size_request(topology,2560,2048);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), topology);
	gtk_container_add(GTK_CONTAINER(box), scrolled_window);
	
	top = GTK_TOPOLOGY(topology);
	top->notify_device = notify_device;
	top->notify_link = notify_link;
}

void init_device_list(GtkWidget *box)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	device_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(device_list), TRUE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Devices", renderer, "text", DEV_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(device_list), column);
	device_store = gtk_list_store_new(DEV_N_COLUMNS, G_TYPE_STRING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(device_list), GTK_TREE_MODEL(device_store));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(device_list));
	g_signal_connect(GTK_OBJECT(device_list), "row-activated", G_CALLBACK(on_changed), NULL);
}

int main(int argc, char **argv)
{
	info = malloc(sizeof(*info));
	hypervisor = malloc(sizeof(*hypervisor));
	config_init(info);
        if (argc == 3) {
                config_read_file(info, argv[1]);
                init_hypervisor(hypervisor,info);
                confdir = argv[2];
        } else {
                init_hypervisor(hypervisor,info);
                confdir = argv[1];
        }
	
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

	init_topology_devices();

	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), G_OBJECT(window));

	gtk_widget_set_events(window, GDK_BUTTON_RELEASE_MASK);
	
	gtk_widget_show_all(window);
	gtk_main();
	
	return 0;
}
