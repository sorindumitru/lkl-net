#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtktopology.h>

#define GRID_SPACING          16

G_DEFINE_TYPE (GtkTopology, gtk_topology, GTK_TYPE_DRAWING_AREA);
 #define GTK_TOPOLOGY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_TOPOLOGY, QuadTree))

// Device draw functions
static void draw_generic(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo);
static void draw_router(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo);

// Events
static gboolean gtk_topology_expose(GtkWidget *topology, GdkEventExpose *event);
static gboolean gtk_topology_button_release(GtkWidget *widget, GdkEventButton *event);
static gboolean gtk_topology_button_press(GtkWidget *widget, GdkEventButton *event);
static gboolean gtk_topology_motion_notify(GtkWidget *widget, GdkEventMotion *event);

/**
 * The device under the mouse
 */
GtkTopologyDevice *under_mouse;

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
	gtk_widget_add_events(GTK_WIDGET(topology), GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	QuadTree *device_tree = GTK_TOPOLOGY_GET_PRIVATE(topology);
	QuadTreeInit(device_tree, 0, 0, 2560, 2048);
}

GtkWidget* gtk_topology_new(void)
{
	GtkWidget *widget = g_object_new(GTK_TYPE_TOPOLOGY, NULL);
	return widget;
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
	if (event->type == GDK_BUTTON_RELEASE) {
		printf("X=%f Y=%f\n", event->x, event->y); 
	}
	
	return FALSE;
}

static gboolean gtk_topology_button_press(GtkWidget *widget, GdkEventButton *event)
{
	QuadTree *device_tree = GTK_TOPOLOGY_GET_PRIVATE(widget);
	if (event->type == GDK_2BUTTON_PRESS) {
		GtkTopologyDevice *device = QuadTreeFindDevice(device_tree, event->x, event->y);
		if (device) {
			printf("Device %s double clicked!\n", device->dev->hostname);
			if (device->dialog) {
				device->dialog(device, widget->window);
			}
		}
	}
	
	return FALSE;
}

static gboolean gtk_topology_motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
	QuadTree *device_tree = GTK_TOPOLOGY_GET_PRIVATE(widget);
	GtkTopologyDevice *device = QuadTreeFindDevice(device_tree, event->x, event->y);

	if (under_mouse != device) {
		under_mouse = device;
		gtk_widget_queue_draw(widget);
	}

	return FALSE;
}

static void draw_generic(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo)
{
	cairo_text_extents_t extents;
	cairo_new_path(cairo);
	cairo_set_source_rgb(cairo, 0.7, 0, 0);
	cairo_set_line_width(cairo, 1.75);
	cairo_arc(cairo, device->dev->x, device->dev->y, 32, 0.0, 2*M_PI);
	cairo_stroke_preserve(cairo);
	cairo_set_source_rgba(cairo, 1.0, 0, 0, 0.4);
	cairo_fill(cairo);
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
	//image = cairo_svg_surface_create("data/router.svg", 72, 72);
	//cairo_set_source_surface(cairo, image, device->dev->x, device->dev->y);
	cairo_paint(cairo);
	cairo_fill(cairo);
	
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

GtkTopologyDevice* gtk_topology_new_hub(device_t *device)
{
	GtkTopologyDevice *hub = malloc(sizeof(*hub));
	memset(hub, 0, sizeof(*hub));
	hub->dev = device;
	hub->xlow = hub->dev->x-32;
	hub->ylow = hub->dev->y-32;
	hub->xhigh = hub->dev->x+32;
	hub->yhigh = hub->dev->y+32;

	return hub;
}

GtkTopologyDevice* gtk_topology_new_router(device_t *device)
{
	GtkTopologyDevice *router = malloc(sizeof(*router));
	memset(router, 0, sizeof(*router));
	router->dev = device;
	router->xlow = router->dev->x-32;
	router->ylow = router->dev->y-32;
	router->xhigh = router->dev->x+32;
	router->yhigh = router->dev->y+32;
	//router->draw = draw_router;
	return router;
}

GtkTopologyDevice* gtk_topology_new_switch(device_t *device)
{
	GtkTopologyDevice *sw = malloc(sizeof(*sw));
	sw->dev = device;
	sw->xlow = sw->dev->x-32;
	sw->ylow = sw->dev->y-32;
	sw->xhigh = sw->dev->x+32;
	sw->yhigh = sw->dev->y+32;
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
