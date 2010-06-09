#ifndef GTK_TOPOLOGY_H_
#define GTK_TOPOLOGY_H_

#include <gtk/gtk.h>

typedef struct _GtkTopology                 GtkTopology;
typedef struct _GtkTopologyClass            GtkTopologyClass;

struct _GtkTopology {
	GtkDrawingArea parent;
};

struct _GtkTopologyClass {
	GtkDrawingAreaClass parent_class;
};

#define GTK_TYPE_TOPOLOGY                  (gtk_topology_get_type())
#define GTK_TOPOLOGY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_TOPOLOGY, GtkTopology))
#define GTK_TOPOLOGY_CLASS(obj)            (G_TYPE_CHECK_CLASS_CAST((obj), GTK_TOPOLOGY, GtkTopologyClass))
#define GTK_IS_TOPOLOGY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_TOPOLOGY))
#define GTK_IS_TOPOLOGY_CLASS(obj)         (G_TYPE_CHECK_CLASS_TYPE((obj), GTK_TYPE_TOPOLOGY))
#define GTK_TOPOLOGY_GET_CLASS             (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_TOPOLOGY, GtkTopologyClass))

#endif /* GTK_TOPOLOGY_H_ */
