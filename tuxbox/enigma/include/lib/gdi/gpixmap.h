#ifndef __gpixmap_h
#define __gpixmap_h

#include <pthread.h>
#include <qstring.h>
#include <qrect.h>
#include "fb.h"
#include "elock.h"

struct gColor
{
	int color;
	gColor(int color): color(color)
	{
	}
	gColor(): color(0)
	{
	}
	operator int() const { return color; }
	int operator ==(const gColor &o) const { return o.color!=color; }
};

struct gRGB
{
	int b, g, r, a;
	gRGB(int r, int g, int b, int a=0): b(b), g(g), r(r), a(a)
	{
	}
	gRGB(unsigned long val): b(val&0xFF), g((val>>8)&0xFF), r((val>>16)&0xFF), a((val>>24)&0xFF)		// ARGB
	{
	}
	gRGB()
	{
	}
};

struct gPalette
{
	int start, len;
	gRGB *data;
};

struct gFont
{
	QString family;
	int pointSize;
	
	gFont(const QString &family, int pointSize):
			family(family), pointSize(pointSize)
	{
	}
	
	gFont()
	{
	}
};


struct gPixmap
{
	int x, y, bpp, bypp, stride;
	void *data;
	int colors;
	gRGB *clut;
	
	eLock contentlock;
	int final;
	
	gPixmap *lock();
	void unlock();
	
	QSize getSize() const { return QSize(x, y); } 
	
	void fill(const QRect &area, const gColor &color);
	void blit(const gPixmap &src, QPoint pos, const QRect &clip=QRect());
	
	void mergePalette(const gPixmap &target);
	void line(QPoint start, QPoint end, gColor color);

	void finalLock();
	gPixmap();
	virtual ~gPixmap();
};

struct gImage: gPixmap
{
	gImage(QSize size, int bpp);
	~gImage();
};

#endif
