#ifndef GTK_TOPOLOGY_H_
#define GTK_TOPOLOGY_H_

#include <list.h>
#include <device.h>
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
	unsigned int xlow, ylow;
	unsigned int xhigh, yhigh;
	unsigned int count;
	struct list_head devices;
	struct quad_tree *children[4];
} QuadTree;

#define SEL_ROUTER    0
#define SEL_SWITCH    1
#define SEL_HUB       2
#define SEL_BRIDGE    3

struct _GtkTopology {	
	GtkDrawingArea parent;
	unsigned char device_sel;

	/**
	 * Devices that are to be shown on the topology
	 * Held both as quad_tree and as list for easy access
	 */
	QuadTree *device_tree;
	struct list_head devices;
	struct list_head links;//List head for links between devices
	
};

struct _GtkTopologyClass {
	GtkDrawingAreaClass parent_class;
};

struct _GtkTopologyDevice {
	unsigned int xlow, ylow;
	unsigned int xhigh, yhigh;
	device_t *dev;
	/**
	 * Function that draws the device
	 */
	void (*draw)(GtkTopologyDevice *device, GtkWidget *topology, cairo_t *cairo);
	/**
	 *
	 */
	void (*dialog)(GtkTopologyDevice *device, GdkWindow *window);
	/**
	 * Function that updates device information from the device
	 */
	void (*update)(GtkTopologyDevice *device);
	struct list_head list;//Used by GtkTopology;
	struct list_head tree;//Used by QuadTree
};

struct _GtkTopologyLink {
	struct _GtkTopologyDevice *end1;
	struct _GtkTopologyDevice *end2;
	struct list_head list;
};

#define GTK_TYPE_TOPOLOGY                  (gtk_topology_get_type())
#define GTK_TOPOLOGY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_TOPOLOGY, GtkTopology))
#define GTK_TOPOLOGY_CLASS(obj)            (G_TYPE_CHECK_CLASS_CAST((obj), GTK_TOPOLOGY, GtkTopologyClass))
#define GTK_IS_TOPOLOGY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_TOPOLOGY))
#define GTK_IS_TOPOLOGY_CLASS(obj)         (G_TYPE_CHECK_CLASS_TYPE((obj), GTK_TYPE_TOPOLOGY))
#define GTK_TOPOLOGY_GET_CLASS             (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_TOPOLOGY, GtkTopologyClass))

GtkWidget* gtk_topology_new();
GtkTopologyDevice* gtk_topology_new_hub(device_t *hhub);
GtkTopologyDevice* gtk_topology_new_router(device_t *hrouter);
GtkTopologyDevice* gtk_topology_new_switch(device_t *hswitch);
void gtk_topology_add_device(GtkTopology *topology, GtkTopologyDevice *device);
void gtk_topology_set_selection(GtkTopology *topology, unsigned char selection);

//QuadTree functions
void QuadTreeInit(QuadTree *tree, unsigned int xlow, unsigned int ylow, unsigned int xhigh, unsigned int yhigh);
void QuadTreeAddDevice(QuadTree *tree, GtkTopologyDevice *device);
GtkTopologyDevice* QuadTreeFindDevice(QuadTree *tree, int x, int y);

#endif /* GTK_TOPOLOGY_H_ */
