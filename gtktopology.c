//TODO: Remove link
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtktopology.h>
#include <interface.h>
#include <math.h>

#define GRID_SPACING          16
#define first_link_end        101
#define second_link_end       102

G_DEFINE_TYPE (GtkTopology, gtk_topology, GTK_TYPE_DRAWING_AREA);
 #define GTK_TOPOLOGY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_TOPOLOGY, QuadTree))

// Device draw functions
static void draw_generic(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo);
static void draw_router(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo);
static void draw_links(GtkTopology *topology, cairo_t *cairo);

// Events
static gboolean gtk_topology_expose(GtkWidget *topology, GdkEventExpose *event);
static gboolean gtk_topology_button_release(GtkWidget *widget, GdkEventButton *event);
static gboolean gtk_topology_button_press(GtkWidget *widget, GdkEventButton *event);
static gboolean gtk_topology_motion_notify(GtkWidget *widget, GdkEventMotion *event);

struct {
	int id;
	char *prefix;
} dev_names[] = {
	{0, "Router"},
	{0, "Switch"},
	{0, "Hub"},
	{0, "Bridge"},
};

struct submenu_data{
	interface_t *interface;
	GtkTopology *top;
};

struct point2D{
	gfloat x;
	gfloat y;
};
/**
 * The device under the mouse
 */
GtkTopologyDevice *under_mouse;
GtkTopologyDevice *drag_device;

GtkTopologyLink *link_device = NULL;
unsigned char hovered = 0;

static void recalc_rect(GtkTopologyDevice *device)
{
	device->xlow = device->dev->x-32;
	device->ylow = device->dev->y-32;
	device->xhigh = device->dev->x+32;
	device->yhigh = device->dev->y+32;
}

static void gtk_topology_class_init(GtkTopologyClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS(class);

	widget_class->expose_event = gtk_topology_expose;
	widget_class->button_release_event = gtk_topology_button_release;
	widget_class->button_press_event = gtk_topology_button_press;
	widget_class->motion_notify_event = gtk_topology_motion_notify;

	g_type_class_add_private(widget_class, sizeof(QuadTree));
	
}

static void gtk_topology_init(GtkTopology *topology)
{
	INIT_LIST_HEAD(&topology->devices);
	INIT_LIST_HEAD(&topology->links);
	topology->device_sel = SEL_NONE;
	gtk_widget_add_events(GTK_WIDGET(topology), GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	QuadTree *device_tree = GTK_TOPOLOGY_GET_PRIVATE(topology);
	QuadTreeInit(device_tree, 0, 0, 2560, 2048);
}

GtkWidget* gtk_topology_new(void)
{
	GtkWidget *widget = g_object_new(GTK_TYPE_TOPOLOGY, NULL);
	return widget;
}

void gtk_topology_add_device_links(GtkTopology *topology, GtkTopologyDevice *device)
{
	struct list_head *i;
	struct list_head *j;
	list_for_each(i,&device->dev->interfaces){
		interface_t *interface;
		interface = list_entry(i,interface_t,list);	
		list_for_each(j,&topology->devices){
			GtkTopologyDevice *d;
			d = list_entry(j, GtkTopologyDevice, list);
			if(interface->link!=NULL && strcmp(interface->link,d->dev->hostname)==0){
				GtkTopologyLink *newLink = malloc(sizeof(GtkTopologyLink));
				INIT_LIST_HEAD(&newLink->list);
				newLink->end1 = device;
				newLink->end2 = d;
				newLink->interface = interface;
				list_add(&newLink->list,&topology->links);
				continue;
			}	
		}
	}
}

void gtk_topology_add_links(GtkTopology *topology)
{
	struct list_head *i;
	list_for_each(i,&topology->devices){
		GtkTopologyDevice *device;
		device = list_entry(i, GtkTopologyDevice, list);
		gtk_topology_add_device_links(topology,device);		
	}
}

void gtk_topology_del_interface_link(GtkTopology *topology,interface_t *del_interface)
{
	struct list_head *i, *tmp;	
	list_for_each_safe(i,tmp,&topology->links){
		GtkTopologyLink *link;
		link = list_entry(i, GtkTopologyLink, list);
		if(del_interface == link->interface){
			del_interface->link = NULL;
			list_del(i);
			link->list.next = link->list.prev = NULL;
			gtk_timeout_add(200,topology->notify_link,link);
			gtk_widget_queue_draw(GTK_WIDGET(topology));
			break;
		}	
	}
	gtk_widget_queue_draw(GTK_WIDGET(topology));
}

struct point2D *versor(GtkTopologyLink *link)
{
	struct point2D *point = malloc(sizeof(struct point2D));
	GtkTopologyDevice *d1 =(link->end1->dev->type != DEV_HUB?link->end1:link->end2);
	GtkTopologyDevice *d2 =(link->end1->dev->type != DEV_HUB?link->end2:link->end1);
	float dist = sqrt(pow(d2->dev->x - d1->dev->x,2)+pow(d1->dev->y - d2->dev->y,2));
	point->x = (d2->dev->x - d1->dev->x)/dist;
	point->y = (d1->dev->y - d2->dev->y)/dist;
	return point; 
	
}

float angle(GtkTopologyLink *link)
{
	GtkTopologyDevice *d1 =(link->end1->dev->type != DEV_HUB?link->end1:link->end2);
	GtkTopologyDevice *d2 =(link->end1->dev->type != DEV_HUB?link->end2:link->end1);
	float cos = (d2->dev->x*d1->dev->x+d2->dev->y*d1->dev->y)/(sqrt(pow(d2->dev->x,2)+pow(d2->dev->y,2))+sqrt(pow(d1->dev->x,2)+pow(d1->dev->y,2)));
	return acos(cos); 
}

static void draw_links(GtkTopology *topology, cairo_t *cairo)
{
	struct list_head *i;
	list_for_each(i,&topology->links){
		GtkTopologyLink *link;
		struct point2D *v;
		cairo_matrix_t matrix;
		int ix,iy;
		link = list_entry(i,GtkTopologyLink,list);
		cairo_set_source_rgb (cairo, 1, 0, 0);
		cairo_set_line_width (cairo, 2.5);
		cairo_new_path(cairo);
		cairo_move_to(cairo,link->end1->dev->x,link->end1->dev->y);
		cairo_line_to(cairo,link->end2->dev->x,link->end2->dev->y);
		cairo_set_font_size(cairo, 16);
		cairo_stroke(cairo);
		
		/*cairo_set_source_rgb (cairo, 0, 0, 0);
		cairo_select_font_face(cairo, "Monospace",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_BOLD);
		v = versor(link);
		if (v->x>0) ix=100;else ix=100;
		if (v->y>0) iy=100;else iy=100;
		//cairo_move_to(cairo, link->end1->dev->x+ix*v->x,link->end1->dev->y+iy*v->y);
		cairo_get_matrix(cairo,&matrix);
		cairo_translate(cairo, 150, 100);
 		cairo_rotate(cairo, angle(link));
		cairo_show_text(cairo, link->interface->dev);
		//cairo_identity_matrix(cairo);
		cairo_set_matrix(cairo,matrix);*/
	}

	cairo_new_path(cairo);
}

static void draw(GtkWidget* widget, cairo_t *cairo)
{
	struct list_head *head;
	int i, max;
	int width = widget->allocation.width;
	int height = widget->allocation.height;
	GtkTopology *topology = GTK_TOPOLOGY(widget);
	
	cairo_set_source_rgb(cairo, 1.0, 0.95, 1);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);

	max = width < height ? height : width;
	cairo_set_line_width (cairo, 0.5);
	cairo_set_source_rgba(cairo, 0, 0, 1, 0.3);
	for (i=0;i<max;i+= GRID_SPACING) {
		if (i < width) {
			cairo_move_to(cairo, i, 0);
			cairo_line_to(cairo, i, height);
		}

		if (i < height) {
			cairo_move_to(cairo, 0, i);
			cairo_line_to(cairo, width, i);
		}
	}
	cairo_stroke(cairo);
	
	draw_links(topology, cairo);

	list_for_each(head, &topology->devices) {
		GtkTopologyDevice *device = list_entry(head, GtkTopologyDevice, list);
		if (device->draw) {
			device->draw(device, widget, cairo);
		} else {
			draw_generic(device, widget, cairo);
		}
	}
}

static gboolean gtk_topology_expose(GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cairo;

	cairo =  gdk_cairo_create(widget->window);

	draw(widget, cairo);

	cairo_destroy(cairo);
	
	return FALSE;
}

static gboolean gtk_topology_button_release(GtkWidget *widget, GdkEventButton *event)
{
	GtkTopology *topology = GTK_TOPOLOGY(widget);
	if (event->type == GDK_BUTTON_RELEASE) {
		if (drag_device){
			if (hovered) {
				drag_device->dev->x = event->x;
				drag_device->dev->y = event->y;
				gtk_widget_queue_draw(widget);
				recalc_rect(drag_device);
			}
			drag_device = NULL;
			hovered = 0;
		} else if (topology->device_sel < SEL_DEL_DEVICE){
			char dev_name[32] = {0};
			device_t *dev = malloc(sizeof(*dev));
			GtkTopologyDevice *device = NULL;
		
			memset(dev, 0, sizeof(*dev));
			sprintf(dev_name,"%s%d", dev_names[topology->device_sel].prefix,
		        dev_names[topology->device_sel].id);
			dev_names[topology->device_sel].id++;
			//TODO:check if name is taken
			//
			dev->x = event->x;
			dev->y = event->y;
			dev->hostname = strdup(dev_name);
			INIT_LIST_HEAD(&dev->interfaces);
			switch(topology->device_sel){
			case SEL_ROUTER:
				dev->type = DEV_ROUTER;
				break;
			case SEL_SWITCH:
				dev->type = DEV_SWITCH;
				break;
			case SEL_HUB:
				dev->type = DEV_HUB;
				break;
			case SEL_BRIDGE:
				dev->type = DEV_BRIDGE;
				break;
			}
			device = gtk_topology_new_hub(dev);
			gtk_timeout_add(200,topology->notify_device,device);
			gtk_topology_add_device(topology, device);
			gtk_widget_queue_draw(widget);
		}
	}
	
	return FALSE;
}
static int not_overriding( void )
{
	struct list_head *i;
	interface_t *interface;
	GtkTopologyDevice *device;
	char *link;
	if (link_device->end2){
		device = (link_device->end1->dev->type != DEV_HUB?link_device->end1:link_device->end2);
		link = strdup(link_device->end1->dev->type == DEV_HUB?link_device->end1->dev->hostname:link_device->end2->dev->hostname);
		list_for_each(i,&device->dev->interfaces){
			interface = list_entry(i,interface_t,list);
			if ( interface->link != NULL && strcmp(interface->link,link)==0 ){
				return 0;
			}
		}
	}
	return 1;
}


static int link_is_valid( void )
{
	if ( not_overriding() && link_device->interface->link == NULL){
		return 1;
	}
	return 0;
}

static void update_interface( void )
{
	if (link_device->end1->dev->type == DEV_HUB){
		link_device->interface->link = strdup(link_device->end1->dev->hostname);
	}else{
		link_device->interface->link = strdup(link_device->end2->dev->hostname);
	}
}

void submenu_clicked(GtkWidget *widget, gpointer data)
{
	struct submenu_data *sdata = (struct submenu_data*)data;
	GtkTopologyDevice *d;
	if (sdata->top->device_sel == SEL_DEL_LINK){
		printf("del link from %s\n",sdata->interface->dev);
		//d = (link_device->end1->dev->type!=DEV_HUB?link_device->end1:link_device->end2);
		gtk_topology_del_interface_link(sdata->top,sdata->interface);
	}else{
		link_device->interface = sdata->interface;
		if (sdata->top->device_sel == SEL_ADD_LINK && link_is_valid()) {
			sdata->top->device_sel = SEL_ADD_LINK2;
		} else{
			if (sdata->top->device_sel == SEL_ADD_LINK2 && link_is_valid()) {
				printf("add new link\n");
				INIT_LIST_HEAD(&link_device->list);
				list_add(&link_device->list,&sdata->top->links);
				sdata->top->device_sel = SEL_ADD_LINK;
				update_interface();
				gtk_timeout_add(200,sdata->top->notify_link,link_device);
			}else
				sdata->top->device_sel = SEL_ADD_LINK;	
		}
	}
}

static void show_popup_menu(GtkTopologyDevice *device,GtkTopology *top)
{
	struct list_head *i;
	GtkWidget *menu;
	menu = gtk_menu_new();
	list_for_each(i,&device->dev->interfaces){
		GtkWidget *sub_menu;
		struct submenu_data *sdata = malloc(sizeof(struct submenu_data));
		sdata->interface = list_entry(i,interface_t,list);
		sub_menu =  gtk_menu_item_new_with_label(sdata->interface->dev);
		sdata->top = top;
		g_signal_connect (GTK_OBJECT (sub_menu), "activate",G_CALLBACK (submenu_clicked), sdata);
		gtk_menu_append(GTK_MENU(menu),sub_menu);
		gtk_widget_show(sub_menu); 	
	}
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 1, 0);
}


static gboolean gtk_topology_button_press(GtkWidget *widget, GdkEventButton *event)
{
	QuadTree *device_tree = GTK_TOPOLOGY_GET_PRIVATE(widget);
	if (event->type == GDK_2BUTTON_PRESS) {
		GtkTopologyDevice *device = QuadTreeFindDevice(device_tree, event->x, event->y);
		if (device) {
			printf("Device %s double clicked!\n", device->dev->hostname);
			if (device->dialog) {
				device->dialog(drag_device, widget->window);
			}
		}
		if (drag_device) {
			drag_device = NULL;
			hovered = 0;
		}
	}

	if (event->type == GDK_BUTTON_PRESS) {
		GtkTopologyDevice *device = QuadTreeFindDevice(device_tree, event->x, event->y);
		GtkTopology *top = GTK_TOPOLOGY(widget);
		if(device){
			if (top->device_sel == SEL_DEL_DEVICE){
				gtk_widget_queue_draw(widget);
				gtk_topology_del_device_links(top, device);
				list_del(&device->list);
				device->list.next = device->list.prev = NULL;
				gtk_timeout_add(200,top->notify_device,device);
				return FALSE;
			} else if (top->device_sel == SEL_ADD_LINK){
				link_device = (GtkTopologyLink*)malloc(sizeof(GtkTopologyLink));
				link_device->end1 = device;
				link_device->end2 = NULL;
				if(device->dev->type == DEV_SWITCH || device->dev->type == DEV_ROUTER){
					show_popup_menu(device,top);
				} else {
					top->device_sel = SEL_ADD_LINK2;
				}
				return FALSE;
			} else if (top->device_sel == SEL_ADD_LINK2){
				if ( (link_device->end1 != device)&&(link_device->end1->dev->type!=device->dev->type)){
					if(link_device->end1->dev->type!= DEV_HUB && device->dev->type == DEV_HUB){
						printf("add new link\n");
						link_device->end2 = device;
						if (not_overriding()){
							INIT_LIST_HEAD(&link_device->list);
							list_add(&link_device->list,&top->links);
							update_interface();
							gtk_timeout_add(200,top->notify_link,link_device);
						}
						top->device_sel = SEL_ADD_LINK;
					}else if(link_device->end1->dev->type==DEV_HUB && device->dev->type != DEV_HUB ){
						link_device->end2 = device;
						show_popup_menu(device,top);
					}
					
				}else
					top->device_sel = SEL_ADD_LINK;
									
				return FALSE;
			}else if (top->device_sel == SEL_DEL_LINK && device->dev->type!=DEV_HUB){
				show_popup_menu(device,top);
			}else {
				top->device_sel = SEL_NONE;
				drag_device = QuadTreeFindDevice(device_tree, event->x, event->y);
			}
		}else if (top->device_sel == SEL_ADD_LINK2){
			top->device_sel = SEL_ADD_LINK;
		}
	}
	
	return FALSE;
}

static void draw_link(GtkWidget *widget, GdkEventMotion *event)
{
	cairo_t *cairo;
	cairo =  gdk_cairo_create(widget->window);
	cairo_set_source_rgb (cairo, 1, 0, 0);
	cairo_set_line_width (cairo, 2.5);
	cairo_new_path(cairo);
	cairo_move_to(cairo,link_device->end1->dev->x,link_device->end1->dev->y);
	cairo_line_to(cairo,event->x,event->y);
	cairo_stroke(cairo);
	cairo_new_path(cairo);
	gtk_widget_queue_draw(widget);
}

static gboolean gtk_topology_motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
	QuadTree *device_tree = GTK_TOPOLOGY_GET_PRIVATE(widget);
	GtkTopologyDevice *device = QuadTreeFindDevice(device_tree, event->x, event->y);
	GtkTopology *top = GTK_TOPOLOGY(widget); 
	if (drag_device){
		drag_device->dev->x = event->x;
		drag_device->dev->y = event->y;
		recalc_rect(drag_device);
		gtk_widget_queue_draw(widget);
		hovered = 1;
	} else if(top->device_sel == SEL_ADD_LINK2){
		draw_link(widget,event);
	} else if (under_mouse != device) {
		under_mouse = device;
		gtk_widget_queue_draw(widget);
	}

	return FALSE;
}

static void draw_generic(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo)
{
	cairo_text_extents_t extents;

	cairo_surface_t *image;

	cairo_new_path(cairo);
	cairo_set_source_rgb(cairo, 1, 1, 1);
	if (device->dev->type == DEV_ROUTER) {
		image = cairo_image_surface_create_from_png("data/router.png");
	} else if (device->dev->type == DEV_HUB) {
		image = cairo_image_surface_create_from_png("data/hub.png");
	} else if (device->dev->type == DEV_SWITCH) {
		image = cairo_image_surface_create_from_png("data/switch.png");
	} else if (device->dev->type == DEV_BRIDGE) {
		image = cairo_image_surface_create_from_png("data/bridge.png");
	} else {
		image = cairo_image_surface_create_from_png("data/generic.png");
	}
	cairo_set_source_surface(cairo, image, device->dev->x-32, device->dev->y-32);
	cairo_paint(cairo);
	
	cairo_set_source_rgb(cairo, 0.2, 0, 0);
	cairo_set_font_size(cairo, 16);
	cairo_select_font_face(cairo, "Monospace",
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_BOLD);
	
	if (device->dev->hostname) {
		cairo_text_extents(cairo, device->dev->hostname, &extents);
		cairo_move_to(cairo, device->dev->x - (extents.width+4)/2, device->dev->y + 36 + extents.height);
		cairo_show_text(cairo, device->dev->hostname);
	} else {
		cairo_text_extents(cairo, "?", &extents);
		cairo_move_to(cairo, device->dev->x - (extents.width+4)/2, device->dev->y + 36 + extents.height);
		cairo_show_text(cairo, "?");
	}

	if (device == under_mouse) {
		cairo_new_path(cairo);
		cairo_set_source_rgb(cairo, 0, 1, 0);
		cairo_set_line_width(cairo, 2.5);
		cairo_rectangle(cairo, device->xlow, device->ylow,
				device->xhigh - device->xlow,
				device->yhigh - device->ylow);
		cairo_stroke(cairo);
	}

}

static void draw_router(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo)
{
	cairo_text_extents_t extents;
	cairo_surface_t *image;

	cairo_new_path(cairo);
	cairo_set_source_rgb(cairo, 1, 1, 1);
	image = cairo_image_surface_create_from_png("data/router.png");
	cairo_set_source_surface(cairo, image, device->dev->x-32, device->dev->y-32);
	cairo_paint(cairo);

	cairo_set_source_rgb(cairo, 0.2, 0, 0);
	cairo_set_font_size(cairo, 16);
	cairo_select_font_face(cairo, "Monospace",
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_BOLD);
	
	if (device->dev->hostname) {
		cairo_text_extents(cairo, device->dev->hostname, &extents);
		cairo_move_to(cairo, device->dev->x - (extents.width+4)/2, device->dev->y + 36 + extents.height);
		cairo_show_text(cairo, device->dev->hostname);
	} else {
		cairo_text_extents(cairo, "?", &extents);
		cairo_move_to(cairo, device->dev->x - (extents.width+4)/2, device->dev->y + 36 + extents.height);
		cairo_show_text(cairo, "?");
	}

	if (device == under_mouse) {
		cairo_new_path(cairo);
		cairo_set_source_rgb(cairo, 0, 1, 0);
		cairo_set_line_width(cairo, 2.5);
		cairo_rectangle(cairo, device->xlow, device->ylow,
				device->xhigh - device->xlow,
				device->yhigh - device->ylow);
		cairo_stroke(cairo);
	}
}

GtkTopologyDevice* gtk_topology_new_generic(device_t *device)
{
	GtkTopologyDevice *router = malloc(sizeof(*router));
	memset(router, 0, sizeof(*router));
	router->dev = device;
	recalc_rect(router);
	return router;
}

GtkTopologyDevice* gtk_topology_new_hub(device_t *device)
{
	GtkTopologyDevice *hub = malloc(sizeof(*hub));
	memset(hub, 0, sizeof(*hub));
	hub->dev = device;
	recalc_rect(hub);

	return hub;
}

GtkTopologyDevice* gtk_topology_new_router(device_t *device)
{
	GtkTopologyDevice *router = malloc(sizeof(*router));
	memset(router, 0, sizeof(*router));
	router->dev = device;
	recalc_rect(router);
	router->draw = draw_router;
	return router;
}

GtkTopologyDevice* gtk_topology_new_switch(device_t *device)
{
	GtkTopologyDevice *sw = malloc(sizeof(*sw));
	memset(sw, 0, sizeof(*sw));
	sw->dev = device;
	recalc_rect(sw);
	return sw;
}

void gtk_topology_add_device(GtkTopology *topology, GtkTopologyDevice *device)
{
	QuadTree *device_tree = GTK_TOPOLOGY_GET_PRIVATE(topology);
	INIT_LIST_HEAD(&device->list);
	INIT_LIST_HEAD(&device->tree);
	list_add(&device->list, &topology->devices);
	QuadTreeAddDevice(device_tree, device);
}

void gtk_topology_del_device_links(GtkTopology *topology, GtkTopologyDevice *device)
{
	struct list_head *head, *temp;

	list_for_each_safe(head, temp, &topology->links) {
		GtkTopologyLink *link = list_entry(head, GtkTopologyLink, list);
		if (link->end1 == device || link->end2 == device) {
			list_del(head);
			free(link);
		}
	}
}

void gtk_topology_set_selection(GtkTopology *topology, unsigned char selection)
{
	topology->device_sel = selection;
}

static char QuadTreeIsLeaf(QuadTree *tree)
{
	return !(tree->children[0] != NULL ||
		tree->children[1] != NULL ||
		tree->children[2] != NULL ||
		tree->children[3] != NULL);
}

static char QuadTreeIsDeviceInNode(QuadTree *tree, GtkTopologyDevice *device)
{
	return (device->dev->x >= tree->xlow &&
		device->dev->x <= tree->xhigh &&
		device->dev->y >= tree->ylow &&
		device->dev->y <= tree->yhigh);
}

static char QuadTreeIsDeviceSelected(GtkTopologyDevice *device, unsigned int x, unsigned int y)
{
	return (x >= device->xlow &&
		x <= device->xhigh &&
		y >= device->ylow &&
		y <= device->yhigh);
}

static void QuadTreeSplit(QuadTree *tree)
{
	int i;
	for (i=0;i<4;i++) {
		unsigned int xlow, ylow, xhigh, yhigh;
		tree->children[i] = malloc(sizeof(QuadTree));
		/**
		 *(xl,yl)
		 *   +---+---+
		 *   | 1 | 2 |
		 *   +___+___+
		 *   | 3 | 4 |
		 *   +___+___+
		 *        (xh,yh)
		 */
		switch(i) {
		case 0:
			xlow = tree->xlow;
			ylow = tree->ylow;
			xhigh = (tree->xhigh-tree->xlow)/2;
			yhigh = (tree->yhigh-tree->ylow)/2;
			break;
		case 1:
			xlow = (tree->xhigh-tree->xlow)/2;
			ylow = tree->ylow;
			xhigh = tree->xhigh;
			yhigh = (tree->yhigh-tree->ylow)/2;
			break;
		case 2:
			xlow = tree->xlow;
			ylow = (tree->xhigh-tree->xlow)/2;
			xhigh = (tree->xhigh-tree->xlow)/2;
			yhigh = tree->yhigh;
			break;
		case 3:
			xlow = (tree->xhigh-tree->xlow)/2;
			ylow = (tree->yhigh-tree->ylow)/2;
			xhigh = tree->xhigh;
			yhigh = tree->yhigh;
			break;
		default:
		        printf("This should not happen. More then 4 children in a tree\n");
			break;
		}
		QuadTreeInit(tree->children[i], xlow, ylow, xhigh, yhigh);
		struct list_head *head, *temp;
		list_for_each_safe(head, temp, &tree->devices) {
			GtkTopologyDevice *device = list_entry(head, GtkTopologyDevice, tree);
			list_del(head);			
			QuadTreeAddDevice(tree->children[i], device);
		}
	}
}

void QuadTreeInit(QuadTree *tree, unsigned int xlow, unsigned int ylow, unsigned int xhigh, unsigned int yhigh)
{
	int i;
	memset(tree, 0, sizeof(*tree));
	INIT_LIST_HEAD(&tree->devices);
	tree->xlow = xlow;
	tree->ylow = ylow;
	tree->xhigh = xhigh;
	tree->yhigh = yhigh;
	tree->count = 0;
	for (i=0;i<4;i++) {
		tree->children[i] = NULL;
	}
}

void QuadTreeAddDevice(QuadTree *tree, GtkTopologyDevice *device)
{
	if (!QuadTreeIsDeviceInNode(tree, device)) {
		return;
	}
	
	if (QuadTreeIsLeaf(tree)) {
		tree->count++;
		INIT_LIST_HEAD(&device->tree);
		list_add(&device->tree, &tree->devices);

		if (tree->count > QUAD_TREE_NODE_THRESHOLD) {
			QuadTreeSplit(tree);
		}
	} else {
		int i;
		for (i=0;i<4;i++) {
			QuadTreeAddDevice(tree->children[i], device);
		}
	}
}

GtkTopologyDevice* QuadTreeFindDevice(QuadTree *tree, int x, int y)
{
	if (QuadTreeIsLeaf(tree)) {
		struct list_head *head;
		list_for_each (head, &tree->devices) {
			GtkTopologyDevice *device = list_entry(head, GtkTopologyDevice, tree);
			if (QuadTreeIsDeviceInNode(tree,device) && QuadTreeIsDeviceSelected(device, x, y)) {
				//TODO: check if mouse is over device
				return device;
			}
		}
	} else {
		int i;
		for (i=0;i<4;i++) {
			GtkTopologyDevice* device = QuadTreeFindDevice(tree->children[i], x, y);
			if (device) {
				return device;
			}
		}
	}

	return NULL;
}
