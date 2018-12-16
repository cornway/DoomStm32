#ifndef GC_DIMENSIONS
#define GC_DIMENSIONS

#include <stdint.h>
#include "mach.h"

#pragma anon_unions

template <typename Range>
    V_PREPACK struct Point {
        union {
              Range x;
              Range w;
            };
            union {
              Range y;
              Range h;
            };  
    };
    
template <typename Range>
    V_PREPACK struct Box {
        Range x;
        Range y;
        Range w;
        Range h;  
    };



template <typename Range>
class Dimension {
    private :
         
    public :
        V_PREPACK struct {
            Range x, y, w, h;
        };
        
        Dimension ()
        {
            this->x = 0;
            this->y = 0;
            this->w = 0;
            this->h = 0;
        }
        Dimension (int x, int y)
        {
            this->x  = x;
            this->y = y;
            this->w = 0;
            this->h = 0;
        }
        Dimension (int x, int y, int w, int h = 0)
        {
            this->x  = x;
            this->y = y;
            this->w = w;
            this->h = h;
        }

        Dimension (Dimension<Range> &d)
        {
            this->x  = d.x;
            this->y = d.y;
            this->w = d.w;
            this->h = d.h;
        }

        template <typename D>
        Dimension (D &d)
        {
            this->x  = d.x;
            this->y = d.y;
            this->w = d.w;
            this->h = d.h;
        }

        template <typename D>
        Dimension (D *d)
        {
            this->x  = d->x;
            this->y = d->y;
            this->w = d->w;
            this->h = d->h;
        }

        void 
        operator () (int x, int y, int w = 0, int h = 0)
        {
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
        }
        void 
        operator () (Box<Range> b)
        {
            this->x = b.x;
            this->y = b.y;
            this->w = b.w;
            this->h = b.h;
        }

        template <typename D>
        Dimension<Range> 
        operator = (D &b)
        {
            this->x  = b.x;
            this->y = b.y;
            this->w = b.w;
            this->h = b.h;
            return *this;
        }

        template <typename D>
        Dimension<Range> 
        operator = (D *b)
        {
            this->x  = b->x;
            this->y = b->y;
            this->w = b->w;
            this->h = b->h;
            return *this;
        }
        
        virtual 
        void 
        setSize (int x, int y, int w = 0, int h = 0)
        {
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
        }
        
        virtual
        void 
        setSize (Dimension<Range> &d)
        {
            this->x = d.x;
            this->y = d.y;
            this->w = d.w;
            this->h = d.h;
        }
        
        virtual
        void 
        setSize (Box<Range> d)
        {
            this->x = d.x;
            this->y = d.y;
            this->w = d.w;
            this->h = d.h;
        }
        
        void 
        setDefaultSize ()
        {
            this->x = 0;
            this->y = 0;
            this->w = 1;
            this->h = 1;
        }
        
        void 
        setX (int x)
        {
            this->x = x;
        }
        
        void 
        setY (int x)
        {
            this->y = x;
        }
        
        virtual
        void 
        setW (int x)
        {
            this->w = x;
        }
        
        virtual
        void 
        setH (int x)
        {
            this->h = x;
        }
        
        void 
        setOrigins (Point<Range> p)
        {
            this->x = p.x - this->w / 2;
            this->y = p.y - this->h / 2;
        }
        
        void 
        setTopLeftCorner (Point<Range> p)
        {
            x = p.x;
            y = p.y - h;
        }
        
        void 
        setTopRightCorner (Point<Range> p)
        {
            x = p.x - w;
            y = p.y - h;
        }
        
        void 
        setBottomLeftCorner (Point<Range> p)
        {
            x = p.x;
            y = p.y;
        }
        
        void 
        setBottomRightCorner (Point<Range> p)
        {
            x = p.x - w;
            y = p.y;
        }
        
        void 
        setTopCenter (Point<Range> p)
        {
            x = p.x - w / 2;
            y = p.y - h;
        }
        
        void 
        setBottomCenter (Point<Range> p)
        {
            x = p.x - w / 2;
            y = p.y;
        }
        
        void 
        setLeftCenter (Point<Range> p)
        {
            x = p.x;
            y = p.y - h / 2;
        }
        void 
        setRightCenter (Point<Range> p)
        {
            x = p.x - w;
            y = p.y - h / 2;
        }
        
        template <typename C>
        void 
        setProportional (C *c, Range wp, Range hp)
        {
            this->w = c->w / wp;
            this->h = c->h / hp;
        }
        
        
        
        int 
        getX ()
        {
            return this->x;
        }
        int 
        getY ()
        {
            return this->y;
        }
        int 
        getWidth ()
        {
            return this->w;
        }
        int 
        getHeight ()
        {
            return this->h;
        }
        Point<Range> &
        getPlacement ()
        {
            Point<Range> p;
            p.x = this->x;
            p.y = this->y;
            return p;
        }
        Point<Range> &
        getSize ()
        {
            static Point<Range> p;
            p.w = this->w;
            p.h = this->h;
            return p;
        }
        
        Point<Range> &
        getTopLeftCorner ()
        {
            static Point<Range> p;
            p.x = x;
            p.y = y + h;
            return p;
        }
        Point<Range> &
        getTopRightCorner ()
        {
            static Point<Range> p;
            p.x = x + w;
            p.y = y + h;
            return p;
        }
        Point<Range> &
        getBottomLeftCorner ()
        {
            static Point<Range> p;
            p.x = x;
            p.y = y;
            return p;
        }
        Point<Range> &
        getBottomRightCorner ()
        {
            static Point<Range> p;
            p.x = x + w;
            p.y = y;
            return p;
        }
        
        Point<Range> &
        getTopCenter ()
        {
            static Point<Range> p;
            p.x = x + w / 2;
            p.y = y + h;
            return p;
        }
        Point<Range> &
        getBottomCenter ()
        {
            static Point<Range> p;
            p.x = x + w / 2;
            p.y = y;
            return p;
        }
        Point<Range> &
        getLeftCenter ()
        {
            static Point<Range> p;
            p.x = x;
            p.y = y + h / 2;
            return p;
        }
        Point<Range> &
        getRightCenter ()
        {
            static Point<Range> p;
            p.x = x + w;
            p.y = y + h / 2;
            return p;
        }
        
        Point<Range> &
        getOrigins ()
        {
            static Point<Range> p;
            p.x = (Range)(this->x + this->w / 2);
            p.y = (Range)(this->y + this->h / 2);
            return p;
        }
        
        Box<Range> 
        getBox ()
        {
            Box<Range> box = {
                    this->x,
                    this->y,
                    this->w,
                    this->h
            };
            return box;
        }
        
        Dimension<Range>
        getDimension ()
        {
            Dimension<Range> d(x, y, w, h);
            return d;
        }
        
        template <typename D>
        bool 
        trunc (D &box)
        {
            if (box.x > this->w) {
                  box.x = this->w;
            }
            if (box.x + box.w > this->w) {
                  box.w = this->w - box.x;
            }
            if (box.y > this->h) {
                  box.y = this->h;
            }
            if (box.y + box.h > this->h) {
                  box.h = this->h - box.y;
            }
            
            //box.x += this->x;
            //box.y += this->y;
            return 0;
        }
        
        bool 
        testPoint (int x, int y)
        {
            int x0 = x - this->x;
            if (x0 < 0) {
                return false;
            }
            if (x0 > this->w) {
                return false;
            }
            int y0 = y - this->y;
            if (y0 < 0) {
                return false;
            }
            if (y0 > this->h) {
                return false;
            }
            return true;
        }
        
        bool 
        testPoint (Point<Range> p) 
        {
            return testPoint(p.x, p.y);
        }
        
        template <typename B>
        bool testInside (B b)
        {
            if (this->testPoint(b.x, b.y) == false) {
                return false;
            }
            if (this->testPoint(b.x + b.w, b.y) == false) {
                return false;
            }
            if (this->testPoint(b.x + b.w, b.y + b.h) == false) {
                return false;
            }
            if (this->testPoint(b.x, b.y + b.h) == false) {
                return false;
            }
            return true;
        }
        
        template <typename B>
        bool testOver (B b)
        {
            if (this->testPoint(b.x, b.y) == true) {
                return true;
            }
            if (this->testPoint(b.x + b.w, b.y) == true) {
                return true;
            }
            if (this->testPoint(b.x + b.w, b.y + b.h) == true) {
                return true;
            }
            if (this->testPoint(b.x, b.y + b.h) == true) {
                return true;
            }
            return false;
        }
        
        template <typename D>
        D &
        normalize (D &d)
        {
            d.x  = d.x - x;
            if (d.x < 0 || d.x > w) {
                d.x = x;
            }
            if (d.x + d.w > w) {
                d.w = w - x;
            }
            d.y  = d.y - y;
            if (d.y < 0 || d.y > h) {
                d.y = y;
            }
            if (d.y + d.h > h) {
                d.h = h - y;
            }   
            return d;
        }
        
        Point<Range> 
        normalize (Range x0, Range y0)
        {
            x0 = x0 - x;
            if (x0 < 0) {
                x0 = x;
            }
            y0 = y0 - x;
            if (y0 < 0) {
                y0 = x;
            }
            Point<Range> p = {x0, y0};
            return p;
        }
        
        Point<Range> 
        normalize (Point<Range> p) 
        {
            return normalize(p.x, p.y);
        }
        
        void moveLeft ()
        {
            this->x = this->x - this->w;
        }
        
        void moveRight ()
        {
            this->x = this->x + this->w;
        }
        
        void moveUp ()
        {
            this->y = this->y + this->h;
        }
        
        void moveDown ()
        {
            this->y = this->y - this->h;
        }
        
};



#endif

/*End of file*/

