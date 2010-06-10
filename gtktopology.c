#include <gtktopology.h>

#define GRID_SPACING          16

G_DEFINE_TYPE (GtkTopology, gtk_topology, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_topology_expose (GtkWidget *topology, GdkEventExpose *event);

static void gtk_topology_class_init(GtkTopologyClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS(class);

	widget_class->expose_event = gtk_topology_expose;
}

static void gtk_topology_init(GtkTopology *topology)
{
	INIT_LIST_HEAD(&topology->devices);
}

GtkWidget* gtk_topology_new(void)
{
	return g_object_new(GTK_TYPE_TOPOLOGY, NULL);
}

static void draw(GtkWidget* topology, cairo_t *cairo)
{
	int i, max;
	int width = topology->allocation.width;
	int height = topology->allocation.height;
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
}

static gboolean gtk_topology_expose(GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cairo;
	struct list_head *head;
	GtkTopology *topology = GTK_TOPOLOGY(widget);

	cairo =  gdk_cairo_create(widget->window);

	draw(topology, cairo);

	cairo_destroy(cairo);

	list_for_each(head, &topology->devices) {
		GtkTopologyDevice *device = list_entry(head, GtkTopologyDevice, list);
		if (device->draw) {
			device->draw(device, topology, cairo);
		}
	}
	
	return FALSE;
}

GtkTopologyDevice* gtk_topology_new_router()
{
	return NULL;
}

GtkTopologyDevice* gtk_topology_new_switch()
{
	return NULL;
}

void gtk_topology_add_device(GtkTopology *topology, GtkTopologyDevice *device)
{
	INIT_LIST_HEAD(&(device->list));
	list_add(&device->list, &topology->devices);
}
