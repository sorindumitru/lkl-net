#ifndef GTK_TOPOLOGY_H_
#define GTK_TOPOLOGY_H_

#include <list.h>
#include <gtk/gtk.h>

typedef struct _GtkTopology                 GtkTopology;
typedef struct _GtkTopologyClass            GtkTopologyClass;
typedef struct _GtkTopologyDevice           GtkTopologyDevice;
typedef struct _GtkTopologyLink             GtkTopologyLink;

#define QUAD_TREE_NODE_THRESHOLD            8 // maximum elements in node;

/**
 * Quad tree data structure for easy finding
 * of devices on click or mouse move
 */
typedef struct quad_tree {
	unsigned int count;
	struct list_head devices;
	struct quad_tree *children[4];
} QuadTree;

struct _GtkTopology {
	GtkDrawingArea parent;

	/**
	 * Devices that are to be shown on the topology
	 * Held both as quad_tree and as list for easy access
	 */
	QuadTree *device_tree;
	struct list_head devices;
};

struct _GtkTopologyClass {
	GtkDrawingAreaClass parent_class;
};

struct _GtkTopologyDevice {
	unsigned int x;
	unsigned int y;
	char *hostname;
	struct list_head links;
	/**
	 * Function that draws the device
	 */
	void (*draw)(GtkTopologyDevice *device, GtkWidget *topology, cairo_t *cairo);
	/**
	 * Function that updates device information from the device
	 */
	void (*update)(GtkTopologyDevice *device);
	struct list_head list;
	struct list_head tree;
};

#define GTK_TYPE_TOPOLOGY                  (gtk_topology_get_type())
#define GTK_TOPOLOGY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_TOPOLOGY, GtkTopology))
#define GTK_TOPOLOGY_CLASS(obj)            (G_TYPE_CHECK_CLASS_CAST((obj), GTK_TOPOLOGY, GtkTopologyClass))
#define GTK_IS_TOPOLOGY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_TOPOLOGY))
#define GTK_IS_TOPOLOGY_CLASS(obj)         (G_TYPE_CHECK_CLASS_TYPE((obj), GTK_TYPE_TOPOLOGY))
#define GTK_TOPOLOGY_GET_CLASS             (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_TOPOLOGY, GtkTopologyClass))

GtkWidget* gtk_topology_new();
GtkTopologyDevice* gtk_topology_new_router();
GtkTopologyDevice* gtk_topology_new_switch();
void gtk_topology_add_device(GtkTopology *topology, GtkTopologyDevice *device);

//QuadTree functions
void QuadTreeInit(QuadTree *tree);
void QuadTreeAddDevice(QuadTree *tree, GtkTopologyDevice *device);
GtkTopologyDevice* QuadTreeFindDevice(QuadTree *tree, int x, int y);

#endif /* GTK_TOPOLOGY_H_ */
