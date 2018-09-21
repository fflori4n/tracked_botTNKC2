// add age to points
// add fadeing of points
// fill polygon finaly
// close polygon

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#include <thread>
#include <vector>

#define angleoffset 30
#define dispx_defofset 2300
#define dispy_defofset 2000

#define mapfile "clear_map.txt"

using namespace std;

struct Point{
	double x;
	double y;
};

vector< vector <Point >> clearborder;	// global couse read map separate funkc.
						
GtkWidget *window;																				// init GTK stuff
GtkWidget *darea;

float disp_xoffset = 0, disp_yoffset = 0, zoom = 5;

float botxpos = 100, botypos = 100, botabsangle = 90 * (3.14/180);

static void do_drawing(cairo_t *);
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr,gpointer user_data){      
  do_drawing(cr);
  return FALSE;
}
bool lines_intersecting(Point& p1, Point& p2, Point& q1, Point& q2) {
    return (((q1.x-p1.x)*(p2.y-p1.y) - (q1.y-p1.y)*(p2.x-p1.x))
            * ((q2.x-p1.x)*(p2.y-p1.y) - (q2.y-p1.y)*(p2.x-p1.x)) < 0)
            &&
           (((p1.x-q1.x)*(q2.y-q1.y) - (p1.y-q1.y)*(q2.x-q1.x))
            * ((p2.x-q1.x)*(q2.y-q1.y) - (p2.y-q1.y)*(q2.x-q1.x)) < 0);
}
vector< vector <Point >> read_map_file(){

	vector< vector <Point >> contours_read;
	vector<Point > onecontour;
	string line;
	string inf;

	clearborder.clear();

	ifstream in(mapfile);
	if(!in){
		cerr << "Cannot open the File "<<endl;
		return(contours_read);
	}
	while (getline(in, line))		// Read the next line from File untill it reaches the end.
	{
		while(line.size() > 0){	
			if(line.find_first_of(" ") == -1){
				inf = line;
				line = "";
			}
			else{
				size_t lastdel = line.find_first_of(" ");
				inf =line.substr(0,(lastdel));
				line =line.substr((lastdel+1),line.size());
				cout<<" -LOOP- "<<endl;
			}
			if(inf.size() > 0){
				//cout<<inf<<"-- "<<endl;
				if(inf.find_first_of(";") != -1){
					try{
						double xcord = stod(inf.substr(0,(inf.find_first_of(";"))));
						double ycord = stod(inf.substr((inf.find_first_of(";") + 1 ),inf.size()));
						cout<<xcord<<" "<<ycord<<endl;
						onecontour.push_back(Point({xcord, ycord}));
					}
					catch(...) {
  						cout<<" Error: String conversion, skipping point "<<endl;
					}
				}
				//cout<<line<<"--"<<endl;
			}
			//sleep(0.8);
		}
		contours_read.push_back(onecontour);
		onecontour.clear();
	}
	in.close();	//Close The File
	return(contours_read);
}
void clean_mapfile(){
	vector< vector <Point >> contours_buff;

	if(read_map_file().size() <= 0)
		return;
	contours_buff = read_map_file();
	/*for(int i=0; i<contours_buff.size(); i++){
		for(int j=0; j < contours_buff[i].size())){
		}
	}*/

}
void do_drawing(cairo_t *cr){

	// draw clear polygons
	// read from file
	if(read_map_file().size() > 0)
		clearborder = read_map_file();
	/*clearborder[0].push_back(Point({0, 0}));
	clearborder[0].push_back(Point({600, 0}));
	clearborder[0].push_back(Point({200, 500}));*/
	float xtotal = (0 + disp_xoffset + dispx_defofset);
	float ytotal = (0 + disp_yoffset + dispy_defofset);
	for(int j = 0; (clearborder.size() != 0 ) && (j < clearborder.size()); j++){
		cairo_set_source_rgba (cr, 0.2, 0.6, 1,0.5);
		for(int i = 0;(clearborder[j].size() != 0) && (i < (clearborder[j].size() - 1)); i++){
			if(i == 0){
				cairo_move_to(cr, (xtotal + clearborder[j][i].x)/(1 + zoom), (ytotal + clearborder[j][i].y)/(1 + zoom));
			}
			cairo_line_to(cr, (xtotal + clearborder[j][i+1].x)/(1 + zoom), (ytotal + clearborder[j][i+1].y)/(1 + zoom));
		}
		cairo_close_path(cr);
		cairo_set_line_width (cr, 0);
		cairo_fill(cr);
		cairo_stroke (cr);
	}

	// view center
	cairo_set_line_width (cr, 2);
	cairo_set_source_rgba (cr, 1, 0, 0,(30));
	cairo_arc(cr, (dispx_defofset)/(6), (dispy_defofset)/(6),max((15/(1 + zoom)),(float)3), 1, 2.3*M_PI);
	cairo_close_path(cr);
	cairo_stroke(cr);
	// origin
	cairo_set_source_rgba (cr, 0, 255, 0,(30));
	cairo_arc(cr,(0 + disp_xoffset + dispx_defofset)/(1 + zoom),(0 + disp_yoffset + dispy_defofset)/(1 + zoom), max((10/(1 + zoom)),(float)5), 1, 2.3*M_PI);
	cairo_close_path(cr);
	cairo_stroke(cr);
	// draw bot orientation line
	float botdispx = (0 + botxpos + disp_xoffset + dispx_defofset)/(1 + zoom);
	float botdispy = (0 + botypos + disp_yoffset + dispy_defofset)/(1 + zoom);
  	cairo_set_source_rgba (cr, 255, 255, 255,(30));
  	cairo_move_to(cr,botdispx, botdispy);
 	cairo_line_to(cr, botdispx + ((600/(1 + zoom)) * cos(botabsangle)),botdispy + ((600/(1 + zoom)) * sin(botabsangle)));
	cairo_stroke(cr);
	// robotpos circles
	for(int i=250; i < 1750; i+=250){															// dist circles 25mm
		cairo_set_source_rgba (cr, 255, 255, 255,(1 - i/250*0.12));
		cairo_arc(cr, (0 + botxpos + disp_xoffset + dispx_defofset)/(1 + zoom), (0 + (-1 * botypos) + disp_yoffset + dispy_defofset)/(1 + zoom), (int(i)/(1 + zoom)), 1, 2.3*M_PI);
		cairo_close_path(cr);
		cairo_stroke(cr);
	}
}
void on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data){
	cairo_t *cr;
	switch (event->keyval){
		case GDK_KEY_Escape:
			//disp_yoffset = 0;
			//disp_xoffset = 0;
			disp_xoffset = botxpos;
			disp_yoffset = botypos;
			zoom = 5;
			break;
		case GDK_KEY_1:
			zoom -= 0.5;
			break;
		case GDK_KEY_2:
			zoom += 0.5;
			break;
    		case GDK_KEY_Up:
			disp_yoffset += (20 + (20 * zoom));
      		break;
		case GDK_KEY_Down:
			disp_yoffset -= (20 + (20 * zoom));
			break;
		case GDK_KEY_Left:
			disp_xoffset += (20 + (20 * zoom));
      		break;
		case GDK_KEY_Right:
			disp_xoffset -= (20 + (20 * zoom));
			break;
    	default:
      		return;
  	}
	do_drawing(cr);
	gtk_widget_queue_draw(window);
}

int main2(){
	// get new points
}
int GUI(int argc, char *argv[]){
	for(;;){
		gtk_init(&argc, &argv);

		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		darea = gtk_drawing_area_new();

		gtk_container_add(GTK_CONTAINER(window), darea);

		g_signal_connect(G_OBJECT(darea), "draw",G_CALLBACK(on_draw_event), NULL); 
		g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL);
		g_signal_connect(G_OBJECT (window), "key_press_event", G_CALLBACK(on_key_press), NULL);
		//g_signal_connect(G_OBJECT (window), "key_press_event", G_CALLBACK(gtk_widget_show_all), NULL);

		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
		gtk_window_set_default_size(GTK_WINDOW(window), 800,800); 
		gtk_window_set_title(GTK_WINDOW(window), "MAP [v]");
		//gtk_window_set_title(GTK_WINDOW(window), "Current [v]");

		gtk_widget_show_all(window);

		gtk_main();
	}
}
int main(int argc, char *argv[])
{
	thread t1(main2);
	thread t2(GUI,argc,argv);
	t1.join();
	t2.join();

	return 0;
}
