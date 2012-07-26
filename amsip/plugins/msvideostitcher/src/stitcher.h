/** Copyright 2007 Simon Morlat, all rights reserved **/

typedef struct _Region{
	float x,y,w;
}Region;

typedef struct _Layout{
	int nregions;
	Region *regions;
}Layout;

