#include <cairo.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gtkmm.h>
#include <glib.h>
#include <opencv2/opencv.hpp>

#define IMGBOX_WIDTH 1280

#define ZOOM_STEP .1f;

float zoom = 1.0f;

#define MAX_WIDTH 1280
#define MAX_HEIGHT 720

#define GLCM_GRAY_SCALE 32

GtkWidget *window, *image, *imagebox;
cv::Mat img;

float clicked_x, clicked_y, released_x, released_y;
float widget_clicked_x, widget_clicked_y, widget_released_x, widget_released_y;

bool training = false;
bool classifying = false;

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

//inicia/para fase de treinamento
void set_training(GtkWidget *widget, gpointer data)
{
  training = !training;
  if (training)
    classifying = false;
}

//inicia/para fase de classificacao
void set_classifying(GtkWidget *widget, gpointer data)
{
  classifying = !classifying;
  if (classifying)
    training = false;
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  if (training || classifying)
  {
    if (training)
      cairo_set_source_rgb(cr, 0.9, 0.5, 0);
    else
      cairo_set_source_rgb(cr, 0.5, 0.9, 0);

    cairo_set_line_width(cr, 3);

    cairo_translate(cr, widget_clicked_x, widget_clicked_y);
    float radius = sqrt(pow(widget_clicked_x - widget_released_x, 2) + pow(widget_clicked_y - widget_released_y, 2));

    cairo_arc(cr, 0, 0, radius, 0, 2 * M_PI);

    cairo_stroke_preserve(cr);
  }

  return FALSE;
}

static gboolean image_clicked (GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
  g_print ("Click nas coordenadas %f,%f\n", event->x, event->y);

  //CORRIGE O DESLOCAMENTO HORIZONTAL DA IMAGEM NO CONTAINER E CALCULA A COORDENADA DADO CERTO ZOOM
  clicked_x = (event->x - IMGBOX_WIDTH/2 + (img.cols * zoom)/2) / zoom;
  clicked_y = event->y / zoom;

  widget_clicked_x = event->x;
  widget_clicked_y = event->y;

  g_print ("Click nas coordenadas da imagem %f,%f\n", clicked_x, clicked_y);

  return TRUE;
}

static gboolean image_click_hover (GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
  if (training || classifying)
  {
    widget_released_x = event->x;
    widget_released_y = event->y;

    gtk_widget_queue_draw(event_box);
  }

  return TRUE;
}

static gboolean image_released (GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
  g_print ("Click solto nas coordenadas %f,%f\n", event->x, event->y);

  //CORRIGE O DESLOCAMENTO HORIZONTAL DA IMAGEM NO CONTAINER E CALCULA A COORDENADA DADO CERTO ZOOM
  released_x = (event->x - IMGBOX_WIDTH/2 + (img.cols * zoom)/2) / zoom;
  released_y = event->y / zoom;

  widget_released_x = event->x;
  widget_released_y = event->y;

  g_print ("Click solto nas coordenadas da imagem %f,%f\n", released_x, released_y);

  //verifica mouse fora dos limites da imagem
  if (clicked_x < 0 || released_x < 0 || clicked_y > (img.rows*zoom) || released_y > (img.rows*zoom))
    return TRUE;

  if (training)
  {
    gtk_widget_queue_draw(event_box);

    GtkDialog *dialog;
    GtkDialogFlags flags = (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
    dialog = (GtkDialog*)gtk_dialog_new_with_buttons ("Esta região corresponde a uma:", (GtkWindow*)window, flags, "_Lesão", GTK_RESPONSE_YES, "_Região sadia", GTK_RESPONSE_NO, NULL);
    gint result = gtk_dialog_run(dialog);

    gtk_widget_destroy((GtkWidget*)dialog);

    if (result != GTK_RESPONSE_DELETE_EVENT)
    {
      bool injury = result == GTK_RESPONSE_YES;

      float energy = 0.0f;
      float contrast = 0.0f;
      float homogenity = 0.0f;
      float entropy = 0.0f;
      int rows = img.rows;
      int cols = img.cols;

      cv::Mat glcm_c1 = cv::Mat::zeros(GLCM_GRAY_SCALE, GLCM_GRAY_SCALE, CV_32FC1);
      cv::Mat glcm_c2 = cv::Mat::zeros(GLCM_GRAY_SCALE, GLCM_GRAY_SCALE, CV_32FC1);
      cv::Mat glcm_c3 = cv::Mat::zeros(GLCM_GRAY_SCALE, GLCM_GRAY_SCALE, CV_32FC1);
      cv::Mat glcm_c5 = cv::Mat::zeros(GLCM_GRAY_SCALE, GLCM_GRAY_SCALE, CV_32FC1);

      cv::Mat gray_image;

      //Converte de RGB pra gray scale
      cvtColor(img, gray_image, cv::COLOR_RGB2GRAY);

      //Reamostra o numero de tons de cinza pra 32
      cv::normalize(gray_image, gray_image, 0, GLCM_GRAY_SCALE, cv::NORM_MINMAX);

      //Calcula as GCLMs ignorando pixels de borda.
      for(int i = 1; i < rows-1; i++)
      {
        for(int j = 0; j < cols-1; j++)
        {
          //checa se o ponto esta dentro do circulo desenhado para treinamento
          cv::Point2f center(clicked_x, clicked_y);
          cv::Point2f point(i,j);

          float circle_radius = sqrt(pow(clicked_x - released_x, 2) + pow(clicked_y - released_y, 2));

          double point_distance_from_center = cv::norm(center - point);

          if (point_distance_from_center <= circle_radius)
          {
            glcm_c1.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i,j+1)) = glcm_c1.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i,j+1)) + 1;
            glcm_c2.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i-1,j+1)) = glcm_c2.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i,j+1)) + 1;
            glcm_c3.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i-1,j)) = glcm_c3.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i,j+1)) + 1;
            glcm_c5.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i+1,j)) = glcm_c5.at<float>(gray_image.at<uchar>(i,j), gray_image.at<uchar>(i,j+1)) + 1;
          }
        }
      }

      //Normaliza as GLCM's, transpondo e dividindo pelo somatorio total
      glcm_c1 = glcm_c1 + glcm_c1.t();
      glcm_c1 = glcm_c1 / sum(glcm_c1)[0];

      glcm_c2 = glcm_c2 + glcm_c2.t();
      glcm_c2 = glcm_c2 / sum(glcm_c2)[0];

      glcm_c3 = glcm_c3 + glcm_c3.t();
      glcm_c3 = glcm_c3 / sum(glcm_c3)[0];

      glcm_c5 = glcm_c5 + glcm_c5.t();
      glcm_c5 = glcm_c5 / sum(glcm_c5)[0];

      //calcula os descritores
      for(int i = 0; i < GLCM_GRAY_SCALE; i++)
      {
        for(int j = 0; j < GLCM_GRAY_SCALE; j++)
        {
          energy = energy + glcm_c1.at<float>(i,j) * glcm_c1.at<float>(i,j);
          energy = energy + glcm_c2.at<float>(i,j) * glcm_c2.at<float>(i,j);
          energy = energy + glcm_c3.at<float>(i,j) * glcm_c3.at<float>(i,j);
          energy = energy + glcm_c5.at<float>(i,j) * glcm_c5.at<float>(i,j);

          contrast = contrast + (i-j) * (i-j) * glcm_c1.at<float>(i,j);
          contrast = contrast + (i-j) * (i-j) * glcm_c2.at<float>(i,j);
          contrast = contrast + (i-j) * (i-j) * glcm_c3.at<float>(i,j);
          contrast = contrast + (i-j) * (i-j) * glcm_c5.at<float>(i,j);

          homogenity = homogenity + glcm_c1.at<float>(i,j) / (1 + abs(i-j));
          homogenity = homogenity + glcm_c2.at<float>(i,j) / (1 + abs(i-j));
          homogenity = homogenity + glcm_c3.at<float>(i,j) / (1 + abs(i-j));
          homogenity = homogenity + glcm_c5.at<float>(i,j) / (1 + abs(i-j));

          if(glcm_c1.at<float>(i,j) != 0)
            entropy = entropy - glcm_c1.at<float>(i,j) * log2(glcm_c1.at<float>(i,j));

          if(glcm_c2.at<float>(i,j) != 0)
            entropy = entropy - glcm_c2.at<float>(i,j) * log2(glcm_c2.at<float>(i,j));

          if(glcm_c3.at<float>(i,j) != 0)
            entropy = entropy - glcm_c3.at<float>(i,j) * log2(glcm_c3.at<float>(i,j));

          if(glcm_c5.at<float>(i,j) != 0)
            entropy = entropy - glcm_c5.at<float>(i,j) * log2(glcm_c5.at<float>(i,j));
        }
      }

      //pegamos a media de cada matriz de co-ocorrencia
      energy = energy / 4.0f;
      contrast = contrast / 4.0f;
      homogenity = homogenity / 4.0f;
      entropy = entropy / 4.0f;

      GtkDialog* result_dialog = (GtkDialog*)gtk_message_dialog_new((GtkWindow*)window, flags, (GtkMessageType)GTK_MESSAGE_INFO, (GtkButtonsType)GTK_BUTTONS_OK, "Homogenidade = %f\nEntropia=%f\nEnergia=%f\nContraste=%f\n", homogenity, entropy, energy, contrast, NULL);

      //Calcula o histograma para informar melhor o usuario
      float range[] = { 0, GLCM_GRAY_SCALE } ;
      const float* hist_range = { range };
      bool uniform = true;
      bool accumulate = false;
      int glcm_gray_scale = GLCM_GRAY_SCALE;

      float circle_radius = sqrt(pow(clicked_x - released_x, 2) + pow(clicked_y - released_y, 2));

      cv::Rect ROI(clicked_x, clicked_y, circle_radius, circle_radius);

      cv::Mat cropped_gray_image = gray_image(ROI);

      cv::Mat hist;
      cv::calcHist(&cropped_gray_image, 1, 0, cv::Mat(), hist, 1, &glcm_gray_scale, &hist_range, uniform, accumulate);

      int hist_w = 512; int hist_h = 400;
      int bin_w = cvRound( (double) hist_w/GLCM_GRAY_SCALE );

      cv::Mat hist_image( hist_h, hist_w, CV_8UC3, cv::Scalar(0,0,0) );

      cv::normalize(hist, hist, 0, hist_image.rows, cv::NORM_MINMAX, -1, cv::Mat() );

      /// Draw for each channel
      for( int i = 1; i < GLCM_GRAY_SCALE; i++ )
      {
          cv::line( hist_image, cv::Point( bin_w*(i-1), hist_h - cvRound(hist.at<float>(i-1)) ) ,
                           cv::Point( bin_w*(i), hist_h - cvRound(hist.at<float>(i)) ),
                           cv::Scalar( 0, 0, 255), 2, 8, 0  );
      }

      /// Display
      cv::namedWindow("Histograma", CV_WINDOW_AUTOSIZE );
      cv::imshow("Histograma", hist_image );

      gtk_dialog_run(result_dialog);
      gtk_widget_destroy((GtkWidget*)result_dialog);

      FILE* fp = fopen("training.csv", "a");
      fprintf(fp, "%d,%f,%f,%f,%f\n", injury ? 1 : 0, homogenity, entropy, energy, contrast);
      fclose(fp);
    }
  }
  else if (classifying)
  {
    gtk_widget_queue_draw(event_box);

    GtkDialog *dialog;
    GtkDialogFlags flags = (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
    dialog = (GtkDialog*)gtk_dialog_new_with_buttons ("Descritores", (GtkWindow*)window, flags, "Próximo", GTK_RESPONSE_YES, NULL);
    GtkWidget *check1 = gtk_toggle_button_new_with_label ("Homogeneidade");
    GtkWidget *check2 = gtk_toggle_button_new_with_label ("Entropia");
    GtkWidget *check3 = gtk_toggle_button_new_with_label ("Energia");
    GtkWidget *check4 = gtk_toggle_button_new_with_label ("Contraste");

    gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check1, FALSE, FALSE, 10);
    gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check2, FALSE, FALSE, 10);
    gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check3, FALSE, FALSE, 10);
    gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check4, FALSE, FALSE, 10);

    gtk_widget_show (check1);
    gtk_widget_show (check2);
    gtk_widget_show (check3);
    gtk_widget_show (check4);

    gint result = gtk_dialog_run(dialog);

    if (result != GTK_RESPONSE_DELETE_EVENT)
    {
      bool homogenity = gtk_toggle_button_get_active ((GtkToggleButton*) check1);
      bool entropy = gtk_toggle_button_get_active ((GtkToggleButton*) check2);
      bool energy = gtk_toggle_button_get_active ((GtkToggleButton*) check3);
      bool contrast = gtk_toggle_button_get_active ((GtkToggleButton*) check4);

      gtk_widget_destroy((GtkWidget*)dialog);

      GtkDialog *dialog2;
      dialog2 = (GtkDialog*)gtk_dialog_new_with_buttons ("Classificador", (GtkWindow*)window, flags, "Classificar", GTK_RESPONSE_YES, NULL);
      GtkWidget *check5 = gtk_toggle_button_new_with_label ("Euclidiano");
      GtkWidget *check6 = gtk_toggle_button_new_with_label ("Vetorial");
      GtkWidget *check7 = gtk_toggle_button_new_with_label ("Mahalanobis");

      gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog2), check5, FALSE, FALSE, 10);
      gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog2), check6, FALSE, FALSE, 10);
      gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog2), check7, FALSE, FALSE, 10);

      gtk_widget_show (check5);
      gtk_widget_show (check6);
      gtk_widget_show (check7);

      result = gtk_dialog_run(dialog2);

      if (result != GTK_RESPONSE_DELETE_EVENT)
      {
        bool euclidian = gtk_toggle_button_get_active ((GtkToggleButton*) check5);
        bool vectorial = gtk_toggle_button_get_active ((GtkToggleButton*) check6);
        bool mahalanobis = gtk_toggle_button_get_active ((GtkToggleButton*) check7);

        gtk_widget_destroy((GtkWidget*)dialog2);
      }
    }
  }
  else
  {
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
  }

  return TRUE;
}

//iniciar fase de treinamento
void visualize_classifier(GtkWidget *widget, gpointer data)
{
  GtkDialog *dialog;
  GtkDialogFlags flags = (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
  dialog = (GtkDialog*)gtk_dialog_new_with_buttons ("Selecione os descritores", (GtkWindow*)window, flags, "_Visualizar", GTK_RESPONSE_YES, NULL);
  GtkWidget *check1 = gtk_toggle_button_new_with_label ("Homogeneidade");
  GtkWidget *check2 = gtk_toggle_button_new_with_label ("Entropia");
  GtkWidget *check3 = gtk_toggle_button_new_with_label ("Energia");
  GtkWidget *check4 = gtk_toggle_button_new_with_label ("Contraste");

  gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check1, FALSE, FALSE, 10);
  gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check2, FALSE, FALSE, 10);
  gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check3, FALSE, FALSE, 10);
  gtk_box_pack_start ((GtkBox*)gtk_dialog_get_content_area(dialog), check4, FALSE, FALSE, 10);

  gtk_widget_show (check1);
  gtk_widget_show (check2);
  gtk_widget_show (check3);
  gtk_widget_show (check4);

  gint result = gtk_dialog_run(dialog);

  if (result != GTK_RESPONSE_DELETE_EVENT)
  {
    bool homogenity = gtk_toggle_button_get_active ((GtkToggleButton*) check1);
    bool entropy = gtk_toggle_button_get_active ((GtkToggleButton*) check2);
    bool energy = gtk_toggle_button_get_active ((GtkToggleButton*) check3);
    bool contrast = gtk_toggle_button_get_active ((GtkToggleButton*) check4);

    gtk_widget_destroy((GtkWidget*)dialog);

    if (homogenity && entropy && energy && contrast)
      return;

    if (!homogenity && !entropy && !energy && !contrast)
      return;

    char params[200];

    sprintf(params, "%s", "ruby plot.rb");

    if (homogenity)
      sprintf(params, "%s %s", params, "1");

    if (entropy)
      sprintf(params, "%s %s", params, "2");

    if (energy)
      sprintf(params, "%s %s", params, "3");

    if (contrast)
      sprintf(params, "%s %s", params, "4");

    g_print(params);

    system(params);
    system("firefox /root/Desktop/pid2017/chart.html");
  }
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
	GtkWidget* trainMi = gtk_menu_item_new_with_label("Treinar/Parar");
	GtkWidget* classifyMi = gtk_menu_item_new_with_label("Classificar/Parar");

  GtkWidget* classifierMi = gtk_menu_item_new_with_label("Classificador");
	GtkWidget* classifierOpenMi = gtk_menu_item_new_with_label("Visualizar");

	//cria hierarquia de menus
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(toolsMi), toolsMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), windowingMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), trainMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), classifyMi);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(classifierMi), classifierMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(classifierMenu), classifierOpenMi);

  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), toolsMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), classifierMi);

	//empacote a barra do menu no container do menu
  gtk_box_pack_start(GTK_BOX(menubox), menubar, FALSE, FALSE, 0);

	image = gtk_image_new_from_file("images/IM_00104.TIF");

  img = cv::imread("images/IM_00104.TIF");
  cv::cvtColor(img, img, CV_BGR2RGB);

  gtk_container_add(GTK_CONTAINER(imagebox), image);

	//Adiciona o container principal na janela
	gtk_container_add(GTK_CONTAINER(window), mainbox);

	//binda os handlers
  gtk_widget_add_events(GTK_WIDGET(imagebox), GDK_SCROLL_MASK);
  g_signal_connect(G_OBJECT(imagebox), "scroll_event", G_CALLBACK(HandleMouseScrollWheel), NULL);
  g_signal_connect(G_OBJECT(imagebox), "button_press_event", G_CALLBACK (image_clicked), image);
  g_signal_connect(G_OBJECT(imagebox), "motion_notify_event", G_CALLBACK (image_click_hover), image);
  g_signal_connect(G_OBJECT(imagebox), "button_release_event", G_CALLBACK (image_released), image);
  g_signal_connect_after(G_OBJECT(imagebox), "draw", G_CALLBACK (on_draw_event), NULL);
	g_signal_connect(G_OBJECT(quitMi), "activate", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(openMi), "activate", G_CALLBACK(load_file), NULL);
  g_signal_connect(G_OBJECT(trainMi), "activate", G_CALLBACK(set_training), NULL);
  g_signal_connect(G_OBJECT(classifyMi), "activate", G_CALLBACK(set_classifying), NULL);
  g_signal_connect(G_OBJECT(windowingMi), "activate", G_CALLBACK(windowing), NULL);
  g_signal_connect(G_OBJECT(classifierOpenMi), "activate", G_CALLBACK(visualize_classifier), NULL);
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
