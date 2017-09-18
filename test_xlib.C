# include <X11/Xlib.h>
# include <unistd.h>
# include <iostream>
# include <aleph.H>


using namespace std;


int window_area(const XWindowAttributes & attr)
{
  return attr.width * attr.height;
}

XWindowAttributes window_attributes(Display * disp, const Window & window)
{
  XWindowAttributes attr;

  int status =  XGetWindowAttributes(disp, window, &attr);

  if (status == 0)
    throw std::domain_error("cannot get window attributes");

  return attr;
}

int window_area(Display * disp, const Window & window)
{
  return window_area(window_attributes(disp, window));
}

void print_window_attributes(const XWindowAttributes & attr)
{
  cout << "(x,y)  = (" << attr.x << "," << attr.y << ")" << endl
       << "width  = " << attr.width << endl
       << "height = " << attr.height << endl;
}


void print_window_attributes(Display * disp, const Window & window)
{
  print_window_attributes(window_attributes(disp, window));
}

Window find_window_with_bigest_area(Display * disp, 
				    Window * ww_array,
				    unsigned int & nwindows)
{
  if (nwindows == 0)
    throw std::domain_error("There are not windows");

  Window max_window = ww_array[0];
  int max_area = window_area(disp, ww_array[0]);

  for (int i = 1; i < nwindows; ++i)
    {
      int current_area = window_area(disp, ww_array[i]);

      if (current_area > max_area)
	{
	  max_area = current_area;
	  max_window = ww_array[i];
	}
    }

  return max_window;
}

int main()
{
// Open a display.
  Display * d = XOpenDisplay(0);

  if (d == NULL)
    EXIT("cannot open dispaly");

  Window default_root_window = DefaultRootWindow(d);

  Window root;
  Window parent;
  Window * children;
  unsigned int nchildren;

  int status = XQueryTree(d, default_root_window, &root, &parent,
			  &children, &nchildren);

  if (status == 0)
    EXIT("error obtaining root window (status = %d)", status);

  cout << "Ventana raíz por omisión es " << default_root_window << endl
       << "que debe ser la misma que " << root << endl
       << "el padre tiene valor " << parent << endl
       << "hay " << nchildren << " ventanas hijas" << endl;

  XWindowAttributes attr;

  status = XGetWindowAttributes(d, root, &attr);

  if (status == 0)
    EXIT("error obtaining root window attributes (status = %d)", status);  

  print_window_attributes(attr);
  
  Window max_window = find_window_with_bigest_area(d, children, nchildren);

  cout << "La ventana con mayor area es " << max_window
       << " con valor de area = " << window_area(d, max_window) << endl;

  print_window_attributes(d, max_window);

  return 0;

}


