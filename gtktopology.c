#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtktopology.h>

#define GRID_SPACING          16

G_DEFINE_TYPE (GtkTopology, gtk_topology, GTK_TYPE_DRAWING_AREA);

// Device draw functions
static void draw_generic(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo);
static void draw_router(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo);

// Events
static gboolean gtk_topology_expose(GtkWidget *topology, GdkEventExpose *event);
static gboolean gtk_topology_button_release(GtkWidget *widget, GdkEventButton *event);
static gboolean gtk_topology_button_press(GtkWidget *widget, GdkEventButton *event);

static void gtk_topology_class_init(GtkTopologyClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS(class);

	widget_class->expose_event = gtk_topology_expose;
	widget_class->button_release_event = gtk_topology_button_release;
	widget_class->button_press_event = gtk_topology_button_press;
}

static void gtk_topology_init(GtkTopology *topology)
{
	INIT_LIST_HEAD(&topology->devices);
	gtk_widget_add_events(GTK_WIDGET(topology), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
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
	cairo_set_source_rgb(cairo, 0.9, 0.95, 1);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);

	max = width < height ? height : width;
	cairo_set_line_width (cairo, 0.5);
	cairo_set_source_rgb(cairo, 0, 0, 0);
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
	return FALSE;
}

static void draw_generic(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo)
{
	cairo_set_source_rgb(cairo, 0.7, 0, 0);
	cairo_set_line_width(cairo, 1.75);
	cairo_arc(cairo, device->x, device->y, 32, 0.0, 2*M_PI);
	cairo_stroke_preserve(cairo);
	cairo_set_source_rgba(cairo, 1.0, 0, 0, 0.4);
	cairo_fill(cairo);
	cairo_set_source_rgb(cairo, 0.2, 0, 0);
	cairo_set_font_size(cairo, 16);
	cairo_select_font_face(cairo, "Monospace",
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_BOLD);
	cairo_move_to(cairo, device->x - 28, device->y + 5);
	if (device->hostname) {
		cairo_show_text(cairo, device->hostname);
	} else {
		cairo_show_text(cairo, "?");
	}
}

static void draw_router(GtkTopologyDevice *device, GtkWidget *widget, cairo_t *cairo)
{
	//GtkTopology *topology = GTK_TOPOLOGY(widget);
}

GtkTopologyDevice* gtk_topology_new_router()
{
	GtkTopologyDevice *router = malloc(sizeof(*router));
	memset(router, 0, sizeof(*router));
	router->hostname = "Router";
	router->x = 100;
	router->y = 100;
	return router;
}

GtkTopologyDevice* gtk_topology_new_switch()
{
	return NULL;
}

void gtk_topology_add_device(GtkTopology *topology, GtkTopologyDevice *device)
{
	INIT_LIST_HEAD(&device->list);
	list_add(&device->list, &topology->devices);
}

static char QuadTreeIsLeaf(QuadTree *tree)
{
	return (tree->children[0] ||
		tree->children[1] ||
		tree->children[2] ||
		tree->children[3]);
}

static void QuadTreeSplit(QuadTree *tree)
{

}

void QuadTreeInit(QuadTree *tree)
{
	memset(tree, 0, sizeof(*tree));
	INIT_LIST_HEAD(&tree->devices);
}

void QuadTreeAddDevice(QuadTree *tree, GtkTopologyDevice *device)
{
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
