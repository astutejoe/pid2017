#include <gtk/gtk.h>
#include <gtkmm.h>
#include <glib.h>
#include <opencv2/opencv.hpp>

#define IMGBOX_WIDTH 1280

#define ZOOM_STEP .1f;

float zoom = 1.0f;

#define MAX_WIDTH 1280
#define MAX_HEIGHT 720

GtkWidget *window, *image, *imagebox;
cv::Mat img;

float clicked_x, clicked_y;

//funcao de utilidade para retornar o caminho de um dialog box do GTK
char* get_file()
{
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Abrir Imagem", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_widget_destroy(dialog);

        return filename;
    }
		else
		{
			gtk_widget_destroy(dialog);
			return NULL;
		}
}

//carrega um novo arquivo, handler para item do menu
void load_file(GtkWidget *widget, gpointer data)
{
    char *file = get_file();

		if (file != NULL) {
	    img = cv::imread(file);
      cv::cvtColor(img, img, CV_BGR2RGB);

	    gtk_image_set_from_file(GTK_IMAGE(image), file);

	    gtk_widget_queue_draw(image);

			free(file);
		}
}

void HandleMouseScrollWheel(GtkWidget *pWidget, GdkEventScroll *event, gpointer pUserData)
{
  if (event->direction == GDK_SCROLL_UP)
  {
    zoom += ZOOM_STEP;

    //CALCULATE A SOFT LIMIT JUST SO THE INTERFACE NEVER BUGS
    if (img.cols * zoom > MAX_WIDTH || img.rows * zoom > MAX_HEIGHT) {
      zoom -= ZOOM_STEP;

      return;
    }

    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(
      img.data,
      GDK_COLORSPACE_RGB,
      false,
      8,
      img.cols,
      img.rows,
      img.step,
      NULL,
      NULL
    );

    pixbuf = gdk_pixbuf_scale_simple(pixbuf, img.cols * zoom, img.rows * zoom, GDK_INTERP_HYPER);

    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
  }
  else if (event->direction == GDK_SCROLL_DOWN)
  {
    zoom -= ZOOM_STEP;

    if (img.rows * zoom <= 0 || img.cols * zoom <= 0) {
      zoom += ZOOM_STEP;

      return;
    }

    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(
      img.data,
      GDK_COLORSPACE_RGB,
      false,
      8,
      img.cols,
      img.rows,
      img.step,
      NULL,
      NULL
    );

    pixbuf = gdk_pixbuf_scale_simple(pixbuf, img.cols * zoom, img.rows * zoom, GDK_INTERP_HYPER);

    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
  }
}

//janelamento
void windowing(GtkWidget *widget, gpointer data)
{
  cv::normalize(img, img, 0, 255, cv::NORM_MINMAX);

  GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(
    img.data,
    GDK_COLORSPACE_RGB,
    false,
    8,
    img.cols,
    img.rows,
    img.step,
    NULL,
    NULL
  );

  gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);

  zoom = 1.0f;
}

static gboolean image_clicked (GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
  g_print ("Click nas coordenadas %f,%f\n", event->x, event->y);

  //CORRIGE O DESLOCAMENTO HORIZONTAL DA IMAGEM NO CONTAINER E CALCULA A COORDENADA DADO CERTO ZOOM
  clicked_x = (event->x - IMGBOX_WIDTH/2 + (img.cols * zoom)/2) / zoom;
  clicked_y = event->y / zoom;

  g_print ("Click nas coordenadas da imagem %f,%f\n", clicked_x, clicked_y);

  return TRUE;
}

static gboolean image_released (GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
  g_print ("Click solto nas coordenadas %f,%f\n", event->x, event->y);

  //CORRIGE O DESLOCAMENTO HORIZONTAL DA IMAGEM NO CONTAINER E CALCULA A COORDENADA DADO CERTO ZOOM
  float released_x = (event->x - IMGBOX_WIDTH/2 + (img.cols * zoom)/2) / zoom;
  float released_y = event->y / zoom;

  g_print ("Click solto nas coordenadas da imagem %f,%f\n", released_x, released_y);

  //verifica mouse fora dos limites da imagem
  if (clicked_x < 0 || released_x < 0 || clicked_y > (img.rows*zoom) || released_y > (img.rows*zoom))
    return TRUE;

  float width = abs(clicked_x - released_x);
  float height = abs(clicked_y - released_y);

  float begin_x = clicked_x < released_x ? clicked_x : released_x;
  float begin_y = clicked_y < released_y ? clicked_y : released_y;

  cv::Rect ROI(begin_x, begin_y, width, height);

  cv::Mat croppedImage = img(ROI);

  GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(
    croppedImage.data,
    GDK_COLORSPACE_RGB,
    false,
    8,
    croppedImage.cols,
    croppedImage.rows,
    croppedImage.step,
    NULL,
    NULL
  );

  gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);

  img = img(ROI);

  zoom = 1.0f;

  return TRUE;
}

int main (int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	//seta propriedades da janela
	gtk_window_set_title(GTK_WINDOW(window), "Trabalho de PID");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_window_set_default_size (GTK_WINDOW (window), 1280, 744);
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);

	//cria os ponteiros pros containers e adiciona o container do menu no container principal
	GtkWidget* mainbox = gtk_box_new(GtkOrientation::GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget* menubox = gtk_box_new(GtkOrientation::GTK_ORIENTATION_HORIZONTAL, 0);
  imagebox = gtk_event_box_new();

  /** DEBUGANDO OS TAMANHOS DOS CONTAINERS /
  GdkColor color;

  gdk_color_parse ("red", &color);
  gtk_widget_modify_bg ( GTK_WIDGET(imagebox), GTK_STATE_NORMAL, &color);

  gdk_color_parse ("blue", &color);
  gtk_widget_modify_bg ( GTK_WIDGET(menubox), GTK_STATE_NORMAL, &color);

  gdk_color_parse ("green", &color);
  gtk_widget_modify_bg ( GTK_WIDGET(mainbox), GTK_STATE_NORMAL, &color);

  /** FIM DEBUG **/

  gtk_container_add(GTK_CONTAINER(mainbox), menubox);
  gtk_container_add(GTK_CONTAINER(mainbox), imagebox);

	//cria os ponteiros para hierarquia do menu
	GtkWidget* menubar = gtk_menu_bar_new();
  GtkWidget* fileMenu = gtk_menu_new();
  GtkWidget* toolsMenu = gtk_menu_new();
  GtkWidget* classifierMenu = gtk_menu_new();

	GtkWidget* fileMi = gtk_menu_item_new_with_label("Arquivo");
	GtkWidget* openMi = gtk_menu_item_new_with_label("Abrir");
	GtkWidget* quitMi = gtk_menu_item_new_with_label("Sair");

  GtkWidget* toolsMi = gtk_menu_item_new_with_label("Ferramentas");
	GtkWidget* windowingMi = gtk_menu_item_new_with_label("Janelamento");
	GtkWidget* collectInjuredMi = gtk_menu_item_new_with_label("Coletar Regiões de Lesões");
	GtkWidget* collectHealthyMi = gtk_menu_item_new_with_label("Coletar Regiões Sadias");

  GtkWidget* classifierMi = gtk_menu_item_new_with_label("Classificador");
  GtkWidget* char1Mi = gtk_check_menu_item_new_with_label("Caracteristica 1");
  GtkWidget* char2Mi = gtk_check_menu_item_new_with_label("Caracteristica 2");
  GtkWidget* char3Mi = gtk_check_menu_item_new_with_label("Caracteristica 3");
  GtkWidget* char4Mi = gtk_check_menu_item_new_with_label("Caracteristica 4");
	GtkWidget* trainMi = gtk_menu_item_new_with_label("Treinar");
  GtkWidget* classify1Mi = gtk_menu_item_new_with_label("Classificar com X");
  GtkWidget* classify2Mi = gtk_menu_item_new_with_label("Classificar com Y");

	//cria hierarquia de menus
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(toolsMi), toolsMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), windowingMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), collectInjuredMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), collectHealthyMi);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(classifierMi), classifierMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), char1Mi);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), char2Mi);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), char3Mi);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), char4Mi);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), trainMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), classify1Mi);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), classify2Mi);

  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), toolsMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), classifierMi);

	//empacote a barra do menu no container do menu
  gtk_box_pack_start(GTK_BOX(menubox), menubar, FALSE, FALSE, 0);

	image = gtk_image_new_from_file("taurus_striker.jpg");

  img = cv::imread("taurus_striker.jpg");
  cv::cvtColor(img, img, CV_BGR2RGB);

  gtk_container_add(GTK_CONTAINER(imagebox), image);

	//Adiciona o container principal na janela
	gtk_container_add(GTK_CONTAINER(window), mainbox);

	//binda os handlers
  gtk_widget_add_events(GTK_WIDGET(imagebox), GDK_SCROLL_MASK);
  g_signal_connect(G_OBJECT(imagebox), "scroll_event", G_CALLBACK(HandleMouseScrollWheel), NULL);
  g_signal_connect(G_OBJECT(imagebox), "button_press_event", G_CALLBACK (image_clicked), image);
  g_signal_connect(G_OBJECT(imagebox), "button_release_event", G_CALLBACK (image_released), image);
	g_signal_connect(G_OBJECT(quitMi), "activate", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(openMi), "activate", G_CALLBACK(load_file), NULL);
  g_signal_connect(G_OBJECT(windowingMi), "activate", G_CALLBACK(windowing), NULL);
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
