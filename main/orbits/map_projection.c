#include "map_projection.h"
#include <math.h>

static const double A_LON = -1.1209751651795787;
static const double B_LON = 159.49405898561318;
static const double A_LAT = -2.4771128058151315; // these values comes from trianguliation the touch screen and the map.
static const double B_LAT = 245.04521964791542;

void map_project_equirect(double lat_deg, double lon_deg, int map_width, int map_height, int *x_px, int *y_px) {
    
    if (!x_px || !y_px || map_width <= 1 || map_height <= 1)
        return;

    double x_raw = A_LON * lon_deg + B_LON;
    double y_raw = A_LAT * lat_deg + B_LAT;  

    int x = (int)lround(x_raw);
    int y = (int)lround(y_raw);

    if (x < 0)
        x = 0;
    if (x >= map_width)
        x = map_width - 1;
    if (y < 0)
        y = 0;
    if (y >= map_height)
        y = map_height - 1;

    *x_px = x;
    *y_px = y;
}
