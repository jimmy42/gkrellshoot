/* --< GKrellShoot 0.4.4 >--{ 8 December 2006 }--
 * Ported to Gkrellm2.0
 *
 * Author: M.R.Muthu Kumar (m_muthukumar@users.sourceforge.net)
 *
 * All the graphic routines are from - Tom Gilbert ( http://linuxbrit.co.uk/ )
 */

#include <gkrellm2/gkrellm.h>

/*
 * Make sure we have a compatible version of GKrellM
 * (Version 2.0+ is required 
 */
#if !defined(GKRELLM_VERSION_MAJOR) \
     || (GKRELLM_VERSION_MAJOR < 2 ) 
#error This plugin requires GKrellM version >= 2.0
#endif

#include<math.h>

#define GKRELLSHOOT_VER 	"0.4.4"
#define GKRELLSHOOT_YEAR 	"2006"
#define DEFAULT_XLOCK		"xscreensaver-command -lock"
#define SHOOT_SCREEN		"import -window root"
#define SHOOT_WINDOW		"import"
#define DEFAULT_VIEW		"display"
#define DEFAULT_IMAGE		"jpg"
#define WINDOW_LABEL	"Grab selected window/area ( unchecking this will grab whole screen)"

#define DEFAULT_OUTFILE	"mk.jpg"
#define	CONFIG_NAME	"GkrellShoot"	/* Name in the configuration window */
#define	STYLE_NAME	"GkrellShoot"	/* Theme subdirectory name */
			                /*  and gkrellmrc style name.*/ 


#define CHART_W 60
#define CHART_H 40

#define MAX_ANIM	11	/* Maximum number of Animations */

#define B_BALL  "Bouncing Ball"
#define MESH    "Mesh"
#define RADAR   "Radar"
#define SINE    "Sine Curve"
#define STAR    "Star Field"
#define RAIN    "Rain"
#define R_LINE  "Random Lines"
#define C_BOARD "Color Board"
#define SCANNER "Scanner"
#define C_BAR   "Color Bars"
#define R_STAR  "Rotating Star"

#define MAX_PANELS	3
#define MIN_PANELS	0
#define NUM_COLORS 32

#define MAX_F_FORMAT	6	/*Maximum filename formats */

#define MMDDYY		"MM-DD-YY"
#define MMDDYYYY	"MM-DD-YYYY"
#define YYMMDD		"YY-MM-DD"
#define YYYYMMDD	"YYYY-MM-DD"
#define DDMMYY		"DD-MM-YY"
#define DDMMYYYY	"DD-MM-YYYY"

#define SELECT_BOTH	0
#define SELECT_LOCK	1
#define SELECT_SHOOT	2

static gchar *anim_name[] = { B_BALL, MESH, RADAR, SINE, STAR, RAIN, R_LINE,
                              C_BOARD, SCANNER, C_BAR, R_STAR };
static gint  current_anim[MAX_PANELS];
static gint  cycle_anim[MAX_PANELS];
static gchar *ff_name[] = { MMDDYY, MMDDYYYY, YYMMDD, YYYYMMDD, DDMMYY, DDMMYYYY };

static GkrellmMonitor		*mon;
static GkrellmChart		*chart[MAX_PANELS];
static GkrellmChartconfig	*chart_config=NULL;

static GkrellmPanel	*panel[3];

static GkrellmDecal	*decal_lock[2],
			*decal_shoot[2];

static gint		chart_w = 60;
static gint		style_id;

static gint wait_seconds;
static gint view_image;
static gint window_or_full;
static gint with_frame;
static gint grayscale;
static gchar xlock_cmd[513];
static gchar view_cmd[513];
static gchar save_dir[513];
static gchar image_format[32];
static gchar ff_select[32];
static gchar shoot_cmd[1024];
static gint  lock_shoot_select = SELECT_BOTH;

static gint active_panels=1;
static gint sel_num_panels=1;
static gboolean panel_visible[MAX_PANELS];



static gchar anim_select[MAX_PANELS][513];


static GtkWidget	*wait_seconds_option, 
                        *view_cmd_option,
                        *image_format_option,
                        *window_option,
                        *anim_select_option[MAX_PANELS],
                        *ff_select_option,
                        *num_panel_option,
                        *cycle_anim_option[MAX_PANELS],
                        *view_image_option,
                        *xlock_cmd_option,
                        *save_dir_option,
                        *frame_option,
                        *grayscale_option,
			*lock_shoot_option[3];

static GtkWidget *laptop = NULL;

static GtkTooltips  *shoot_tips = 0;
static gchar        *shoot_tips_text;

static gint r_g_b[][3] = { 
     	                 	{ 0  , 139, 139 },
       	                 	{ 144, 238, 144 },
                         	{ 138, 43 , 226 },
                         	{ 216, 191, 216 },
                         	{ 255, 0  , 255 },
                         	{ 238, 130, 238 },
                         	{ 255, 105, 180 },
                         	{ 255, 20 , 147 },
                         	{ 255, 0  , 0   },
                         	{ 255, 165, 0   },
                         	{ 210, 105, 30  },
                         	{ 178, 34 , 34  },
                         	{ 222, 184, 135 },
                         	{ 139, 69 , 19  },
                         	{ 188, 143, 143 },
                         	{ 205, 92 , 92  },
                         	{ 255, 255, 0   },
                         	{ 255, 215, 0   },
                         	{ 240, 230, 140 },
                         	{ 189, 183, 107 },
                         	{ 220, 220, 220 },
                         	{ 255, 228, 225 },
                         	{ 190, 190, 190 },
                         	{ 230, 230, 250 },
                         	{ 100, 149, 137 },
                         	{ 100, 149, 237 },
                         	{ 0  , 191, 255 },
                         	{ 123, 104, 238 },
                         	{ 224, 255, 255 },
                         	{ 0  , 255, 255 },
                         	{ 152, 251, 152 },
                         	{ 46 , 139, 87  }
                         };

gchar filename[1024];
int load_val = 50;
struct tm  *tm;

GkrellmTicks *gk_ticks;


guchar *rgbbuf_t[MAX_PANELS];

static void change_num_panels ();

/* Set a pixel, takes a brightness and a colour value */
static void
set_col_pixel (gint x, gint y,  guchar c,  guchar rrr,
                guchar ggg,  guchar bbb, gint chart_index)
{
  guchar *ptr;

  if ((((int) c) == 0) || (x < 0) || (y < 0) || (x > chart_w - 1) || (y > 39))
    return;
 

  ptr = rgbbuf_t[chart_index] + ( chart_w * 3 * (y)) + (3 * x);
  ptr[0] = ((double) rrr / 255 * (double) c);
  ptr[1] = ((double) ggg / 255 * (double) c);
  ptr[2] = ((double) bbb / 255 * (double) c);
}
static void color_buf( gint chart_index, guchar rrr, guchar ggg, guchar bbb  )
{
  guchar *pos;
  gint y, x;

  pos = rgbbuf_t[chart_index];
  for (y = 0; y < CHART_H; y++)
    {
      for (x = 0; x < chart_w; x++)
        {

          pos[0] = rrr ;		/* Red */
          pos[1] = ggg ;		/* Green */
          pos[2] = bbb ;		/* Blue */
          pos += 3;
        }
    }

}

static void fade_buf( gint per, gint chart_index )
{
  guchar *pos;
  gint y, x;

  pos = rgbbuf_t[chart_index];
  for (y = 0; y < CHART_H; y++)
    {
      for (x = 0; x < chart_w; x++)
        {

          pos[0] = pos[0] * per / 100;		/* Red */
          pos[1] = pos[1] * per / 100;		/* Green */
          pos[2] = pos[2] * per / 100;		/* Blue */
          pos += 3;
        }
    }

}

static void
blank_buf( gint chart_index )
{
  guchar *pos;
  gint x,y;

  pos = rgbbuf_t[chart_index];
  for (y = 0; y < CHART_H; y++)
    {
      for (x = 0; x < chart_w; x++)
        {

          pos[0] = pos[1] = pos[2] = 0;
          pos += 3;
        }
    }
}
/* Scrolls buf along one to the left */
static void
scroll_buf ( gint chart_index )
{
  gint x, y;
  guchar *rgb, *rptr;

  for (y = 0; y < 40; y++)
    {
      rgb = rgbbuf_t[chart_index];
      rptr = rgb + (y * chart_w * 3);
      for (x = 0; x < chart_w - 1; x++)
        {
          rptr[0] = rptr[3];
          rptr[1] = rptr[4];
          rptr[2] = rptr[5];
          rptr += 3;
        }
      rptr[0] = 0;
      rptr[1] = 0;
      rptr[2] = 0;
    }
}

static gint 
get_rand_num()
{
   gint j;

  j=1+(gint) (255.0*rand()/(RAND_MAX+1.0));

  return j;
}
/* Bouncing ball */
static void
draw_ball ( gint chart_index )
{
  /* following static array variables must be initialized upto MAX_PANELS */
  static gint setup[] = { 0,0,0 };
  static gdouble ball_x[MAX_PANELS], ball_y[MAX_PANELS];
  static gdouble d_x[MAX_PANELS], d_y[MAX_PANELS];

  static gint col_index[] = { 0,0,0 };

  gint i;

  if (!setup[chart_index])
    {
      ball_x[chart_index] = rand () % 39;
      ball_y[chart_index] = rand () % 39;
      while (abs (d_x[chart_index]) < 0.5)
        d_x[chart_index] = ((double) rand () / RAND_MAX * 4) - 2.0;
      while (abs (d_y[chart_index]) < 0.5)
        d_y[chart_index] = ((double) rand () / RAND_MAX * 4) - 2.0;
      blank_buf ( chart_index );
      setup[chart_index] = 1;
    }
  ball_x[chart_index] += d_x[chart_index];
  ball_y[chart_index] += d_y[chart_index];

  if (ball_x[chart_index] < 1)
    {
      ball_x[chart_index] = 1;
      d_x[chart_index] = -d_x[chart_index];
      col_index[chart_index]=(gint) (33.0*rand()/(RAND_MAX+1.0));
    }
  else if (ball_x[chart_index] > chart_w - 3)
    {
      ball_x[chart_index] = chart_w - 3;
      d_x[chart_index] = -d_x[chart_index];
      col_index[chart_index]=(gint) (33.0*rand()/(RAND_MAX+1.0));
    }

  if (ball_y[chart_index] < 1)
    {
      ball_y[chart_index] = 1;
      d_y[chart_index] = -d_y[chart_index];
      col_index[chart_index]=(gint) (33.0*rand()/(RAND_MAX+1.0));
    }
  else if (ball_y[chart_index] > 37)
    {
      ball_y[chart_index] = 37;
      d_y[chart_index] = -d_y[chart_index];
      col_index[chart_index]=(gint) (33.0*rand()/(RAND_MAX+1.0));
    }

  if ( col_index[chart_index] >= NUM_COLORS ) { col_index[chart_index] = 0; }

  i = col_index[chart_index];

  fade_buf (90, chart_index );
  set_col_pixel (ball_x[chart_index], ball_y[chart_index], 255, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
  set_col_pixel (ball_x[chart_index] + 1, ball_y[chart_index], 255, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
  set_col_pixel (ball_x[chart_index], ball_y[chart_index] + 1, 255, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
  set_col_pixel (ball_x[chart_index] + 1, ball_y[chart_index] + 1, 155, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );

  set_col_pixel (ball_x[chart_index] - 1, ball_y[chart_index], 255, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
  set_col_pixel (ball_x[chart_index], ball_y[chart_index] - 1, 255, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
  set_col_pixel (ball_x[chart_index] - 1, ball_y[chart_index] - 1, 155, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
  set_col_pixel (ball_x[chart_index] - 1, ball_y[chart_index] + 1, 155, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
  set_col_pixel (ball_x[chart_index] + 1, ball_y[chart_index] - 1, 155, 
                 r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], chart_index );
}

#define RADIUS 50

static void draw_radar( gint chart_index )
{
   gint i, tmp_w;
   guint tempX = 0;
   guint tempY = 0;

  /* following static array variables must be initialized upto MAX_PANELS */
   static gint setup[] = { 0,0,0 };

   static gdouble radar_x[MAX_PANELS][RADIUS];
   static gdouble radar_r[MAX_PANELS][RADIUS];

   gdouble d = 0.0;

   if ( !setup[chart_index] )
   {
     d = rand() % 360;

     for ( i = 0; i < RADIUS; i++ )
     {
       radar_x[chart_index][i] = d;
       radar_r[chart_index][i] = i;
     }

     setup[chart_index] = 1;
   }

   fade_buf( 92, chart_index );

    tmp_w = chart_w / 2;
    for (i = 0; i < RADIUS; i++)
    {
      radar_x[chart_index][i] += 0.07;

      tempX = (tmp_w - 1) + radar_r[chart_index][i] * cos (radar_x[chart_index][i]) / 3;
      tempY = 19 + radar_r[chart_index][i] * sin (radar_x[chart_index][i]) / 3;

      set_col_pixel (tempX, tempY, 255, 55, 255, 75, chart_index);
    }
}

/* Sine Curve */

static void
draw_sine ( gint chart_index )
{
  /* following static array variables must be initialized upto MAX_PANELS */
  static gdouble x[] = { 0,0,0 };
  static gint setup[] = { 0,0,0 };

  if (!setup[chart_index])
    {
      blank_buf ( chart_index );
      setup[chart_index] = 1;
    }
  else
    scroll_buf ( chart_index );

  set_col_pixel (chart_w - 1, 19 + 14 * sin (x[chart_index]), 255, 115, 255, 165, 
                 chart_index );
  x[chart_index] += 0.3;
}

/* Star Field */

#define NUM_STARS 300
double star_x[NUM_STARS];
double star_y[NUM_STARS];
double star_z[NUM_STARS];
double star_zv[NUM_STARS];

double star_screenx[NUM_STARS];
double star_screeny[NUM_STARS];

static void
draw_starfield ( chart_index )
{
  gint i;
  guchar b;

  /* following static array variables must be initialized upto MAX_PANELS */
  static gint setup[] = { 0,0,0 };

  if (!setup[chart_index])
    {
      for (i = 1; i < NUM_STARS; i++)
        {
          /* Never got the hang of random numbers under unix... */
          /* This can't be the way to do it... */
          star_x[i] = ((double) rand () / RAND_MAX * 2000) - 1000.0;
          star_y[i] = ((double) rand () / RAND_MAX * 2000) - 1000.0;
          star_z[i] = ((double) rand () / RAND_MAX * 600) + 400.0;
          star_zv[i] = (((double) rand () / RAND_MAX * 45) + 5) / 10;
        }
      setup[chart_index] = 1;
    }
  blank_buf ( chart_index );

  for (i = 1; i < NUM_STARS; i++)
    {
      star_z[i] = star_z[i] - star_zv[i];

      star_screenx[i] = star_x[i] / star_z[i] * 100 + 19;
      star_screeny[i] = star_y[i] / star_z[i] * 100 + 19;

      if ((star_screenx[i] > chart_w - 1) || (star_screenx[i] < 0)
          || (star_screeny[i] > 39) || (star_screeny[i] < 0)
          || (star_z[i] < 1))
        {
          star_x[i] = ((double) rand () / RAND_MAX * 2000) - 1000.0;
          star_y[i] = ((double) rand () / RAND_MAX * 2000) - 1000.0;
          star_z[i] = ((double) rand () / RAND_MAX * 600) + 400.0;
          star_zv[i] = (((double) rand () / RAND_MAX * 45) + 5) / 10;
        }
      else
        {
          b = ((255 / 5) * star_zv[i]) * (1 - (star_z[i] / 1000));
          set_col_pixel (star_screenx[i], star_screeny[i], b, 255, 250, 250, chart_index );
          set_col_pixel (star_screenx[i] + 1, star_screeny[i], b, 255, 250,
                         250, chart_index );
          set_col_pixel (star_screenx[i], star_screeny[i] + 1, b, 255, 250,
                         250, chart_index );
          set_col_pixel (star_screenx[i] + 1, star_screeny[i] + 1, b, 255,
                         250, 250, chart_index );
        }
    }
}

static void
aa_line (gint x1, gint y1, gint x2, gint y2, guchar b,
         guchar rr, guchar gg, guchar bb, gint chart_index )
{
  gdouble grad, line_width, line_height, xgap, ygap, xend, yend, yf, xf,
    brightness1, brightness2, db, xm, ym;
  gint ix1, ix2, iy1, iy2, i;
  gint temp;

  guchar c1, c2;

  line_width = (x2 - x1);
  line_height = (y2 - y1);

  if (abs (line_width) > abs (line_height))
    {
      if (x1 > x2)
        {
          temp = x1;
          x1 = x2;
          x2 = temp;
          temp = y1;
          y1 = y2;
          y2 = temp;
          line_width = (x2 - x1);
          line_height = (y2 - y1);
        }

      /* This is currently broken. It is supposed to account
       * for lines that don't span more than one pixel */
      if (abs (line_width) < 0.1)
        {
          x2 = x1 + 0.5;
          x1 -= 0.5;
          grad = 0;
        }
      else
        {
          grad = line_height / line_width;
          if (line_width < 1)
            {
              xm = (x1 + x2) / 2;
              ym = (y1 + y2) / 2;

              x1 = xm - 0.5;
              x2 = xm + 0.5;
              y1 = ym - (grad / 2);
            y2 = ym + (grad / 2);

              line_width = 1;
              line_height = grad;
            }
        }

      xend = (int) x1 + 0.5;
      yend = y1 + grad * (xend - x1);

      xgap = (1 - modf (x1 + 0.5, &db));
      ix1 = (int) xend;
      iy1 = (int) yend;

      brightness1 = (1 - modf (yend, &db)) * xgap;
      brightness2 = modf (yend, &db) * xgap;

      c1 = (unsigned char) (brightness1 * b);
      c2 = (unsigned char) (brightness2 * b);

      set_col_pixel (ix1, iy1, c1, rr, gg, bb, chart_index );
      set_col_pixel (ix1, iy1 + 1, c2, rr, gg, bb, chart_index );

      yf = yend + grad;

      xend = (int) (x2 + .5);
      yend = y2 + grad * (xend - x2);

      xgap = 1 - modf (x2 - .5, &db);

      ix2 = (int) xend;
      iy2 = (int) yend;

      brightness1 = (1 - modf (yend, &db)) * xgap;
      brightness2 = modf (yend, &db) * xgap;

      c1 = (unsigned char) (brightness1 * b);
      c2 = (unsigned char) (brightness2 * b);

      set_col_pixel (ix2, iy2, c1, rr, gg, bb, chart_index );
      set_col_pixel (ix2, iy2 + 1, c2, rr, gg, bb, chart_index );

      for (i = ix1 + 1; i < ix2; i++)
        {
          brightness1 = (1 - modf (yf, &db));
          brightness2 = modf (yf, &db);
          c1 = (unsigned char) (brightness1 * b);
          c2 = (unsigned char) (brightness2 * b);

          set_col_pixel (i, (int) yf, c1, rr, gg, bb, chart_index );
          set_col_pixel (i, (int) yf + 1, c2, rr, gg, bb, chart_index );

          yf = yf + grad;
        }
    }
  else
    {
      if (y2 < y1)
        {
          temp = x1;
          x1 = x2;
          x2 = temp;
          temp = y1;
          y1 = y2;
          y2 = temp;
          line_width = (x2 - x1);
          line_height = (y2 - y1);
        }

      /* This is currently broken */
      if (abs (line_height) < 0.1)
        {
          y2 = y1 + 0.5;
          y1 -= 0.5;
          grad = 0;
        }
      else
        {
          grad = line_width / line_height;
          if (line_height < 1)
            {
              xm = (x1 + x2) / 2;
              ym = (y1 + y2) / 2;

              x1 = xm - (grad / 2);
              x2 = xm + (grad / 2);
              y1 = ym - 0.5;
              y2 = ym + 0.5;

              line_height = 1;
              line_width = grad;
            }
        }

      yend = (int) (y1 + 0.5);
      xend = x1 + grad * (yend - y1);

      ygap = (1 - modf (y1 + 0.5, &db));
      ix1 = (int) xend;
      iy1 = (int) yend;

      brightness1 = (1 - modf (xend, &db)) * ygap;
      brightness2 = modf (xend, &db) * ygap;

      c1 = (unsigned char) (brightness1 * b);
      c2 = (unsigned char) (brightness2 * b);

      set_col_pixel (ix1, iy1, c1, rr, gg, bb, chart_index );
      set_col_pixel (ix1 + 1, iy1, c2, rr, gg, bb, chart_index );

      xf = xend + grad;

      yend = (int) (y2 + .5);
      xend = x2 + grad * (yend - y2);

      ygap = 1 - modf (y2 - .5, &db);

      ix2 = (int) xend;
      iy2 = (int) yend;

      brightness1 = (1 - modf (xend, &db)) * ygap;
      brightness2 = modf (xend, &db) * ygap;

      c1 = (unsigned char) (brightness1 * b);
      c2 = (unsigned char) (brightness2 * b);

      set_col_pixel (ix2, iy2, c1, rr, gg, bb, chart_index );
      set_col_pixel (ix2 + 1, iy2, c2, rr, gg, bb, chart_index );
      for (i = iy1 + 1; i < iy2; i++)
        {
          brightness1 = (1 - modf (xf, &db));
          brightness2 = modf (xf, &db);

          c1 = (unsigned char) (brightness1 * b);
          c2 = (unsigned char) (brightness2 * b);

          set_col_pixel ((int) xf, i, c1, rr, gg, bb, chart_index );
          set_col_pixel ((int) (xf + 1), i, c2, rr, gg, bb, chart_index );

          xf += grad;
        }
    }
}
static gdouble ox[49];
static gdouble oy[49];
static gdouble oz[49];
static gdouble x[49];
static gdouble y[49];
static gdouble z[49];

static void
draw_rotator ( gint chart_index )
{
  gint xx = (chart_w / 2 ) - 1;
  gint yy = 19;

  /* following static array variables must be initialized upto MAX_PANELS */

  static gint setup[] = { 0,0,0 };
  static gdouble theta1[] = { 0.00,0.00,0.00 };
  static gdouble theta2[] = { 0.00,0.00,0.00 };
  static gdouble theta3[] = { 0.00,0.00,0.00 };

  gdouble theta1inc = 0.05;
  gdouble theta2inc = 0.10;
  gdouble theta3inc = 0.03;
  gint i;
  gdouble tx, ty;
  gdouble xcopy, ycopy, zcopy;
  gint from[84] =
    { 0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 21, 22,
    23, 24, 25, 26, 28, 29, 30, 31, 32, 33, 35, 36, 37, 38, 39, 40, 42, 43,
    44, 45, 46, 47, 0, 7, 14, 21, 28, 35, 1, 8, 15, 22, 29, 36, 2, 9, 16,
    23, 30, 37, 3, 10, 17, 24, 31, 38, 4, 11, 18, 25, 32, 39, 5, 12, 19, 26,
    33, 40, 6, 13, 20, 27, 34, 41
  };
  gint to[84] =
    { 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20, 22, 23,
    24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 36, 37, 38, 39, 40, 41, 43, 44,
    45, 46, 47, 48, 7, 14, 21, 28, 35, 42, 8, 15, 22, 29, 36, 43, 9, 16, 23,
    30, 37, 44, 10, 17, 24, 31, 38, 45, 11, 18, 25, 32, 39, 46, 12, 19, 26,
    33, 40, 47, 13, 20, 27, 34, 41, 48
  };

  if (!setup[chart_index])
    {
      for (i = 0; i < 49; i++)
        {
          x[i] = 0;
          y[i] = 0;
          z[i] = 0;

          ox[i] = ((i % 7) - 3) * 75;
          oy[i] = ((i / 7) - 3) * 75;
          tx = (i % 7) - 3;
          ty = (i / 7) - 3;
          oz[i] = 50 - (-9 * tx * tx + 9 * ty * ty);
        }
      setup[chart_index] = 1;
    }
  theta1[chart_index] += theta1inc;
  theta2[chart_index] += theta2inc;
  theta3[chart_index] += theta3inc;

  blank_buf ( chart_index );

  for (i = 0; i < 49; i++)
    {
      x[i] = ox[i];
      y[i] = oy[i];
      z[i] = oz[i];

      xcopy = x[i];
      x[i] = (x[i] * cos (theta1[chart_index]) - y[i] * sin (theta1[chart_index]));
      y[i] = (xcopy * sin (theta1[chart_index]) + y[i] * cos (theta1[chart_index]));

      ycopy = y[i];
      y[i] = (y[i] * cos (theta2[chart_index]) - z[i] * sin (theta2[chart_index]));
      z[i] = (ycopy * sin (theta2[chart_index]) + z[i] * cos (theta2[chart_index]));

      zcopy = z[i];
      z[i] = (z[i] * cos (theta3[chart_index]) - x[i] * sin (theta3[chart_index]));
      x[i] = (zcopy * sin (theta3[chart_index]) + x[i] * cos (theta3[chart_index]));

      /* add perspective */
      x[i] = (29 * x[i] / (550 - z[i]));
      y[i] = (29 * y[i] / (550 - z[i]));
    }

  for (i = 0; i < 84; i++)
    {
      aa_line (xx + x[from[i]], yy + y[from[i]],
               xx + x[to[i]], yy + y[to[i]], (unsigned char) 255, 0, 255,
               255, chart_index );
    }
}

static void 
draw_rain( gint chart_index )
{
   gint i, tmp_w;

  /* following static array variables must be initialized upto MAX_PANELS */
   static gint j[] = { 0,0,0 };
   static gint first[] = { 0,0,0 };

   gint k = 0;

   static gint red = 55;
   static gint green = 255;
   static gint blue  = 75;

   tmp_w = chart_w / 6;
   fade_buf( 95, chart_index );

   if ( first[chart_index] == 0 )
   {
      first[chart_index] = 1;

      for ( i = 0; i <  tmp_w; i=i+3 )
      {
         set_col_pixel (i, j[chart_index], 255, red, green, blue, chart_index);
      }
      for ( i = tmp_w * 2; i <  tmp_w * 3; i=i+3 )
      {
         set_col_pixel (i, j[chart_index], 255, red, green, blue, chart_index);
      }
      for ( i = tmp_w * 4; i <  tmp_w * 5; i=i+3 )
      {
         set_col_pixel (i, j[chart_index], 255, red, green, blue, chart_index);
      }
   }
   else
   {
     if ( first[chart_index] == 1 )
     {
        first[chart_index] = 0;

        for ( i = tmp_w; i <  tmp_w * 2; i=i+3 )
        {
           set_col_pixel (i, j[chart_index], 255, red, green, blue, chart_index);
        }
        for ( i = tmp_w * 3; i <  tmp_w * 4; i=i+3 )
        {
           set_col_pixel (i, j[chart_index], 255, red, green, blue, chart_index);
        }
        for ( i = tmp_w * 5; i <  tmp_w * 6; i=i+3 )
        {
           set_col_pixel (i, j[chart_index], 255, red, green, blue, chart_index);
        }
     }
   }

   j[chart_index] = j[chart_index] + 2;

  if ( j[chart_index] >=39 )
  {
    j[chart_index] = 0;
    k=1+(gint) (4.0*rand()/(RAND_MAX+1.0));

    switch( k ) 
    {
      case 1:
      {
        red = 55;
        green = 255;
        blue  = 75;
        break;
      }
      case 2:
      {
        red = 255;
        green = 0;
        blue  = 0;
        scroll_buf( chart_index );
        break;
      }
      case 3:
      {
        red = 255;
        green = 0;
        blue  = 255;
        break;
      }
      default:
      {
        red = 102;
        green = 255;
        blue  = 255;
        scroll_buf( chart_index );
        break;
      }
    }
  }
}
/* Star */
void
draw_rstar ( gint chart_index )
{
  gint x1, y1;
  gint x2, y2;
  gint x3, y3;
  gint x4, y4;
  gint x5, y5;
  gint x6, y6;

  /* following static array variables must be initialized upto MAX_PANELS */
  static gint do_scroll[] = { 0,0,0 };
  static gint draw_count[] = { 0,0,0 };
  static gint zoom_count[] = { 0,0,0 };
  static gint col_index[] = { 0,0,0 };
  static gint r1[] = { 10,10,10 };
  static double x[]  = { 0.0,0.0,0.0 };

  gint radius=0;

  gint j, tmp_w;


  if ( draw_count[chart_index] >= 95 ) 
  {
     if ( zoom_count[chart_index] == 0 ) { draw_count[chart_index] = 0; }

     r1[chart_index] = r1[chart_index] + 5;

	 if ( r1[chart_index] > 40 && zoom_count[chart_index] <= 10 )
	 {
       draw_count[chart_index] = 95;
	   r1[chart_index] = 10; 

	   zoom_count[chart_index]++;

	   if ( zoom_count[chart_index] >= 10 ) { r1[chart_index] = 45; }
	 }
  }

  if ( zoom_count[chart_index] >= 10 ) { zoom_count[chart_index] = 0; }

  if ( r1[chart_index] > 40  && do_scroll[chart_index] > 0 
        && do_scroll[chart_index] < chart_w - 10 )
  { 
     draw_count[chart_index] = 0;
	 do_scroll[chart_index]++;
	 scroll_buf( chart_index );
	 return;
  }
  if ( do_scroll[chart_index] > chart_w - 11 )
  {
     do_scroll[chart_index] = 0;
     draw_count[chart_index] = 0;
	 r1[chart_index] = 10; 

     col_index[chart_index]=(gint) (33.0*rand()/(RAND_MAX+1.0));
	 if ( col_index[chart_index] >= NUM_COLORS ) { col_index[chart_index] = 0; }
  }

  j = col_index[chart_index];

  radius=r1[chart_index] * load_val/100;

  x[chart_index] += 0.04;

  tmp_w = (chart_w / 2 ) - 1;
  x1 = tmp_w + radius * cos (x[chart_index]) ;
  y1 = 19 + radius * sin (x[chart_index]) ;
  x2 = tmp_w + radius * cos (x[chart_index] + (2 * M_PI / 3)) ;
  y2 = 19 + radius * sin (x[chart_index] + (2 * M_PI / 3)) ;
  x3 = tmp_w + radius * cos (x[chart_index] + (4 * M_PI / 3)) ;
  y3 = 19 + radius * sin (x[chart_index] + (4 * M_PI / 3)) ;

  x4 = tmp_w + radius * cos (x[chart_index] + (M_PI / 3)) ;
  y4 = 19 + radius * sin (x[chart_index] + (M_PI / 3)) ;
  x5 = tmp_w + radius * cos (x[chart_index] + (3 * M_PI / 3)) ;
  y5 = 19 + radius * sin (x[chart_index] + (3 * M_PI / 3)) ;
  x6 = tmp_w + radius * cos (x[chart_index] + (5 * M_PI / 3)) ;
  y6 = 19 + radius * sin (x[chart_index] + (5 * M_PI / 3)) ;

  fade_buf (80, chart_index);

  aa_line (x1, y1, x2, y2, 255, r_g_b[j][0], r_g_b[j][1], 
	         r_g_b[j][2], chart_index);
  aa_line (x2, y2, x3, y3, 255, r_g_b[j][0], r_g_b[j][1], 
	         r_g_b[j][2], chart_index);
  aa_line (x3, y3, x1, y1, 255, r_g_b[j][0], r_g_b[j][1], 
	         r_g_b[j][2], chart_index);

  aa_line (x4, y4, x5, y5, 255, r_g_b[j][0], r_g_b[j][1], 
	         r_g_b[j][2], chart_index);
  aa_line (x5, y5, x6, y6, 255, r_g_b[j][0], r_g_b[j][1], 
	         r_g_b[j][2], chart_index);
  aa_line (x6, y6, x4, y4, 255, r_g_b[j][0], r_g_b[j][1], 
	         r_g_b[j][2], chart_index);

  do_scroll[chart_index] = 1;
  draw_count[chart_index]++;
}


static void
draw_colorbar ( gint chart_index )
{
  static gint setup[] = { 0,0,0 };
  static gint y[] = { 39,39,39 };
  static gint y2[] = { 39,39,39 };
  static gint s_val[] = { 0,0,0 };
  static gint col_index[] = { 0,0,0 };
  static gboolean start_y2[] = { FALSE,FALSE,FALSE };

  gint i,y2_col;

  if (!setup[chart_index])
    {
      blank_buf ( chart_index );
      setup[chart_index] = 1;
    }
  else { scroll_buf ( chart_index ); }

  i = col_index[chart_index];

  y2_col = i + 1;
  if ( y2_col >= NUM_COLORS ) { y2_col = 0; }


  if ( s_val[chart_index] > 25 )
  {
    fade_buf ( 85, chart_index );
    s_val[chart_index] = 0;

	y[chart_index] = y[chart_index] - 2;
	if ( y[chart_index] <= 0 ) 
	{ 
      color_buf( chart_index, r_g_b[i][0], r_g_b[i][1], r_g_b[i][2] );
	  y[chart_index] = 39; 
	}
    col_index[chart_index]=(gint) (33.0*rand()/(RAND_MAX+1.0));
	if ( col_index[chart_index] >= NUM_COLORS ) { col_index[chart_index] = 0; }

    if ( y[chart_index] < 19 && ! start_y2[chart_index] ) 
	{ 
	  start_y2[chart_index] = TRUE; 
	}

    if ( start_y2[chart_index] ) 
	{ 
	  y2[chart_index] = y2[chart_index] - 2; 
	}

	if ( y2[chart_index] <= 0 ) 
	{
	  start_y2[chart_index] = FALSE; 
	  y2[chart_index] = 39; 
	}
  }


  set_col_pixel (chart_w - 1, y[chart_index], 255, r_g_b[i][0], r_g_b[i][1], 
                     r_g_b[i][2], chart_index);
  set_col_pixel (chart_w - 1, y[chart_index]-1, 255, r_g_b[i][0], r_g_b[i][1], 
                     r_g_b[i][2], chart_index);

  if ( y[chart_index] <= 19 || y2[chart_index] < 20 ) 
  {
    set_col_pixel (chart_w - 1, y2[chart_index], 255, r_g_b[y2_col][0], 
	                   r_g_b[y2_col][1], r_g_b[y2_col][2], chart_index);
    set_col_pixel (chart_w - 1, y2[chart_index]-1, 255, r_g_b[y2_col][0], 
	                   r_g_b[y2_col][1], r_g_b[y2_col][2], chart_index);
  }

  s_val[chart_index]++;
}

/* Scanner */
static void
draw_scanner (gint chart_index)
{
  gint y = 0, y2 = 0;
  gint x = 0, x2 = 0;
  /* following static array variables must be initialized upto MAX_PANELS */
  static gint setup[] = { 0,0,0 };
  static gint load_lr[] = { 100,100,100 };
  static gboolean l_add[] = { FALSE,FALSE,FALSE };
  static gboolean v_scan[] = { FALSE,FALSE,FALSE };
  static gint col_index[] = { 0,0,0 };
  static gint minute_timer[] = { 0,0,0 }; 


  gint i, tmp_w;

  if (!setup[chart_index])
  {
      blank_buf ( chart_index );
      setup[chart_index] = 1;
  }

  if (gk_ticks->minute_tick && ++minute_timer[chart_index] >= 2 )
  {
    minute_timer[chart_index] = 0;

    col_index[chart_index]=(gint) (33.0*rand()/(RAND_MAX+1.0));
	if ( col_index[chart_index] >= NUM_COLORS ) { col_index[chart_index] = 0; }

	if ( v_scan[chart_index] ) { v_scan[chart_index] = FALSE; }
	else { v_scan[chart_index] = TRUE; }
  }

  i = col_index[chart_index];

  fade_buf (95, chart_index);

  tmp_w = chart_w - 3;
  if ( v_scan[chart_index] )
  {
    x = tmp_w-(tmp_w*load_lr[chart_index]/100) +1;
    x2 = tmp_w-(tmp_w*load_lr[chart_index]/100) +1;

    aa_line (x, 0, x2, 39, 255, r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], 
           chart_index); 
  }
  else
  {
    y = 37-(37*load_lr[chart_index]/100) +1;
    y2 = 37-(37*load_lr[chart_index]/100) +1;

    aa_line (0, y, chart_w - 1, y2, 255, r_g_b[i][0], r_g_b[i][1], r_g_b[i][2], 
           chart_index); 
  }

  if ( l_add[chart_index] ) { load_lr[chart_index]++; }
  else { load_lr[chart_index]--; }

  if ( load_lr[chart_index] <=0  ) { l_add[chart_index] = TRUE; }

  if ( load_lr[chart_index] >=100 ) { l_add[chart_index] = FALSE; }
}


/* Color Board */

void draw_cboard(gint chart_index)
{
  /* following static array variables must be initialized upto MAX_PANELS */
  static gint col_count[] = { 0,0,0 };
  static gint do_fade[] = { 0,0,0 };
  static gint col_index[] = { 0,0,0 };

  gint i;

  if ( col_count[chart_index] >= 30 && do_fade[chart_index] > 0 && 
       do_fade[chart_index] < 20 )
  {
    fade_buf( 95, chart_index );
	do_fade[chart_index]++;
	return;
  }
  if ( do_fade[chart_index] > 19 )
  {
    do_fade[chart_index] = 0;
	col_count[chart_index] = 0;

	col_index[chart_index]++;
	if ( col_index[chart_index] >= NUM_COLORS ) { col_index[chart_index] = 0; }
  }
  
  i = col_index[chart_index];

  color_buf( chart_index, r_g_b[i][0], r_g_b[i][1], r_g_b[i][2] );

  col_count[chart_index]++;
  do_fade[chart_index] = 1;
}
void draw_rline(gint chart_index)
{
   guint x1 = 0;
   guint y1 = 0;

   guint x2 = 0;
   guint y2 = 0;

   guint x3 = 0;
   guint y3 = 0;

   gint red = 0;
   gint green = 0;
   gint blue = 0;

  /* following static array variables must be initialized upto MAX_PANELS */
   static gint do_scroll[] = { 0,0,0 };
   static gint draw_count[] = { 0,0,0 };
   static gint fade_or_scroll[] = { 0,0,0 };

   if ( draw_count[chart_index] >= 75 && do_scroll[chart_index] > 0 
        && do_scroll[chart_index] < chart_w )
   {
     if ( fade_or_scroll[chart_index] == 0 ) 
     { 
       fade_buf( 95, chart_index );
       do_scroll[chart_index]++;
     }
     else { scroll_buf( chart_index ); }

     do_scroll[chart_index]++;
     return;
   }
   if ( do_scroll[chart_index] > chart_w - 1 )
   {
     do_scroll[chart_index] = 0;
     draw_count[chart_index] = 0;
     if ( fade_or_scroll[chart_index] ) { fade_or_scroll[chart_index] = 0; }
     else { fade_or_scroll[chart_index] = 1; }
   }

   red = get_rand_num();
   green = get_rand_num();
   blue = get_rand_num();


   if ( fade_or_scroll[chart_index] ) { fade_buf( 95, chart_index ); }

   x1 = rand() % chart_w;
   y1 = rand() % CHART_H;

   x2 = rand() % chart_w;
   y2 = rand() % CHART_H;

   aa_line (x1, y1, x2, y2, (unsigned char) 255, red, green, blue, chart_index);

   x3 = rand() % chart_w;
   y3 = rand() % CHART_H;
   aa_line (x2, y2, x3, y3, (unsigned char) 255, red, green, blue, chart_index);

   x2 = rand() % chart_w;
   y2 = rand() % CHART_H;
   aa_line (x3, y3, x2, y2, (unsigned char) 255, red, green, blue, chart_index);

   aa_line (x2, y2, x1, y1, (unsigned char) 255, red, green, blue, chart_index);

   do_scroll[chart_index] = 1;
   draw_count[chart_index]++;
}


static gint
chart_expose_event(GtkWidget *widget, GdkEventExpose *ev, gpointer echart )
{
  gint i;

  i = GPOINTER_TO_INT( echart );

  if ( (i + 1) > active_panels ) { return TRUE ; }
  if (widget != chart[i]->drawing_area) { return TRUE; }

  if ( ! strcmp( anim_select[i], B_BALL ) ) { draw_ball( i ); }
  if ( ! strcmp( anim_select[i], MESH ) ) { draw_rotator( i ); }
  if ( ! strcmp( anim_select[i], RADAR ) ) { draw_radar( i ); }
  if ( ! strcmp( anim_select[i], SINE ) ) { draw_sine( i ); }
  if ( ! strcmp( anim_select[i], STAR ) ) { draw_starfield( i ); }
  if ( ! strcmp( anim_select[i], RAIN ) ) { draw_rain( i ); }
  if ( ! strcmp( anim_select[i], R_LINE ) ) { draw_rline( i ); }
  if ( ! strcmp( anim_select[i], C_BOARD ) ) { draw_cboard( i ); }
  if ( ! strcmp( anim_select[i], SCANNER ) ) { draw_scanner( i ); }
  if ( ! strcmp( anim_select[i], C_BAR ) ) { draw_colorbar( i ); }
  if ( ! strcmp( anim_select[i], R_STAR ) ) { draw_rstar( i ); }


   gdk_draw_rgb_image (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                      0, 0, chart_w, CHART_H,
                      GDK_RGB_DITHER_MAX, rgbbuf_t[i], chart_w * 3);
   return TRUE;
}

static gint
expose_event (GtkWidget *widget, GdkEventExpose *ev, gpointer epanel)
{
  gint i;

  i = GPOINTER_TO_INT( epanel );

    if (widget == panel[i]->drawing_area)
    {
        gdk_draw_pixmap(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
            panel[i]->pixmap, ev->area.x, ev->area.y, ev->area.x, ev->area.y,
            ev->area.width, ev->area.height);
    }
    return FALSE;
}

static void
anim_skip_dup( gint chart_index )
{
   gint i;

   for ( i = 0; i < active_panels; i++ )
   {
     if ( i == chart_index) { continue; }
	 if ( current_anim[chart_index] == current_anim[i] )
	 {
       current_anim[chart_index]++;
	 }
   }
}
static void
switch_anim( gint chart_index, gboolean allowDup )
{
   current_anim[chart_index]++;
   if ( ! allowDup) { anim_skip_dup( chart_index ); }

   if( current_anim[chart_index] >= MAX_ANIM ) 
   { 
     current_anim[chart_index] = 0; 
   }

   fade_buf (90, chart_index );
   strcpy( anim_select[chart_index], anim_name[current_anim[chart_index]] );
}

static gint
panel_press (GtkWidget *widget, GdkEventButton *ev )
{
    if (ev->button == 3)
    {
       gkrellm_open_config_window(mon);
    }
    return TRUE;
}

static gint
anim_chart_press (GtkWidget *widget, GdkEventButton *ev, gpointer echart)
{
    gint i;

    i = GPOINTER_TO_INT( echart );

    if (ev->button == 2)
    {
       switch_anim( i, TRUE );
    }
    else if (ev->button == 3)
    {
       gkrellm_open_config_window(mon);
    }
    return TRUE;
}


static void
run_xlock_cmd()
{
  if ( xlock_cmd )
  {
     g_spawn_command_line_async( xlock_cmd, NULL );
  }
  return;
}

static void
update_cycle_anim( gint i )
{
  static gint minute_timer[] = { 0,0,0 }; // shoud have MAX_PANELS array size.

  if (gk_ticks->minute_tick && ++minute_timer[i] >= cycle_anim[i] )
  {
    switch_anim(i, FALSE);
    minute_timer[i] = 0;
  }
}
static void
update_plugin ()
{

  gint i = 0;

  GdkEventExpose event;
  gint ret_val;


    for ( i = 0; i < MAX_PANELS; i++ )
    {
	  if ( cycle_anim[i] > 0 ) { update_cycle_anim( i ); }

      g_signal_emit_by_name(GTK_OBJECT(chart[i]->drawing_area),
                                "expose_event", &event, &ret_val  );
    }
}

static void
make_shoot_cmd(void)
{
   gchar tmp_wcmd[512];
   gchar tmp_vcmd[512];

   gchar tmp_scmd[32];
   gchar tmp_fcmd[32];
   gchar tmp_gcmd[32];

   if ( wait_seconds > 0 )
   {
     sprintf( tmp_scmd, "sleep %d &&", wait_seconds );
   }
   else { sprintf( tmp_scmd, "%s", " " ); }

   if ( with_frame ) { sprintf( tmp_fcmd, " %s ", "-frame" ); }
   else { sprintf( tmp_fcmd, "%s", " " ); }

   if ( grayscale ) 
   { 
     sprintf( tmp_gcmd, " %s ", "-colorspace GRAY -depth 8" ); 
   }
   else { sprintf( tmp_gcmd, "%s", " " ); }

   if ( window_or_full ) 
   {
     sprintf( tmp_wcmd,"%s %s %s %s ",  SHOOT_WINDOW, tmp_fcmd, tmp_gcmd, filename );
   }
   else 
   {
     sprintf( tmp_wcmd,"%s %s %s %s ",  SHOOT_SCREEN, tmp_fcmd, tmp_gcmd, filename );
   }

   if ( view_image )
   {
     sprintf( tmp_vcmd," && %s %s ",  view_cmd, filename );
   }
   else
   {
     strcpy( tmp_vcmd, " " );	   
   }	   

   sprintf( shoot_cmd,"%s %s %s &",  tmp_scmd, tmp_wcmd, tmp_vcmd );

}

static void
run_shoot_cmd()
{

  if ( strlen( image_format ) == 0 )
  {
    strcpy( image_format, "jpg" );
  }

  tm = gkrellm_get_current_time();

if ( ! strcmp( ff_select, YYMMDD ) ) {
  sprintf( filename, "%s/gkrellShoot_%02d-%02d-%02d_%02d%02d%02d.%s", 
		  save_dir,
		  tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, 
		  tm->tm_min, tm->tm_sec, image_format );
} else  if ( ! strcmp( ff_select, YYYYMMDD ) ) {
  sprintf( filename, "%s/gkrellShoot_%02d-%02d-%02d_%02d%02d%02d.%s", 
		  save_dir,
		  tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, 
		  tm->tm_min, tm->tm_sec, image_format );
} else  if ( ! strcmp( ff_select, DDMMYY ) ) {
  sprintf( filename, "%s/gkrellShoot_%02d-%02d-%02d_%02d%02d%02d.%s", 
		  save_dir,
		  tm->tm_mday, tm->tm_mon+1,  tm->tm_year-100, tm->tm_hour, 
		  tm->tm_min, tm->tm_sec, image_format );
} else  if ( ! strcmp( ff_select, DDMMYYYY ) ) {
  sprintf( filename, "%s/gkrellShoot_%02d-%02d-%02d_%02d%02d%02d.%s", 
		  save_dir,
		  tm->tm_mday, tm->tm_mon+1,  tm->tm_year+1900, tm->tm_hour, 
		  tm->tm_min, tm->tm_sec, image_format );
} else  if ( ! strcmp( ff_select, MMDDYYYY ) ) {
  sprintf( filename, "%s/gkrellShoot_%02d-%02d-%02d_%02d%02d%02d.%s", 
		  save_dir,
		  tm->tm_mon+1, tm->tm_mday, tm->tm_year+1900, tm->tm_hour, 
		  tm->tm_min, tm->tm_sec, image_format );
} else {
  sprintf( filename, "%s/gkrellShoot_%02d-%02d-%02d_%02d%02d%02d.%s", 
		  save_dir,
		  tm->tm_mon+1, tm->tm_mday, tm->tm_year-100, tm->tm_hour, 
		  tm->tm_min, tm->tm_sec, image_format );
}

  
  make_shoot_cmd();

  if ( shoot_cmd )
  {
     system ( shoot_cmd );
  }
  return;
}

static void
cb_button(GkrellmDecalbutton *button)
{
  if (GPOINTER_TO_INT(button->data) == 0)
  {
    run_xlock_cmd();
  }
  if (GPOINTER_TO_INT(button->data) == 1)
  {
    run_shoot_cmd();
  }
}

static void
show_lock_shoot_select()
{

   if ( lock_shoot_select == SELECT_BOTH )
   {
      gkrellm_panel_show( panel[SELECT_BOTH] );
      gkrellm_panel_hide( panel[SELECT_LOCK] );
      gkrellm_panel_hide( panel[SELECT_SHOOT] );

   }
   if ( lock_shoot_select == SELECT_LOCK )
   {
      gkrellm_panel_show( panel[SELECT_LOCK] );
      gkrellm_panel_hide( panel[SELECT_BOTH] );
      gkrellm_panel_hide( panel[SELECT_SHOOT] );
   }
   if ( lock_shoot_select == SELECT_SHOOT )
   {
      gkrellm_panel_show( panel[SELECT_SHOOT] );
      gkrellm_panel_hide( panel[SELECT_BOTH] );
      gkrellm_panel_hide( panel[SELECT_LOCK] );
   }
}

static void
create_plugin(GtkWidget *vbox, gint first_create)
{
	GkrellmStyle			*style;
	GkrellmTextstyle		*ts, *ts_alt;

	gint			x,y, i;
	gint			tmp_w;

    
	if (first_create)
	{
          for ( i=0; i < 3; i++ )
          {
	    panel[i] = gkrellm_panel_new0();
          }

	  for ( i=0; i < MAX_PANELS; i++ )
	  {
	    chart[i] = gkrellm_chart_new0();
	  }
	}  
	else 
	{ 
          for ( i=0; i < 3; i++ )
          {
	     gkrellm_destroy_krell_list(panel[i]);
	     gkrellm_destroy_decal_list(panel[i]); 
          }
	}

	for ( i=0; i < MAX_PANELS; i++ )
        {	
	  gkrellm_set_chart_height_default(chart[i], 40);
	  gkrellm_chart_create(vbox, mon, chart[i], &chart_config);
	}

	style = gkrellm_meter_style(style_id);

	/* Each Style has two text styles.  The theme designer has picked the
	|  colors and font sizes, presumably based on knowledge of what you draw
	|  on your panel.  You just do the drawing.  You probably could assume
	|  the ts font is larger than the ts_alt font, but again you can be
	|  overridden by the theme designer.
	*/
	ts = gkrellm_meter_textstyle(style_id);
	ts_alt = gkrellm_meter_alt_textstyle(style_id);
        for ( i=0; i < 3; i++ )
        {
	  panel[i]->textstyle = ts;/* would be used for a panel label */
        }

       /* Create a text decal that will be converted to a button.  
	* The "Lock" string is not an initialization string it is just a
	* sizing string.
        */
        decal_lock[0] = gkrellm_create_decal_text(panel[0], "Lock", ts_alt, style, 2, 2, 0);

	x = decal_lock[0]->x + decal_lock[0]->w + 6;
	y = decal_lock[0]->y + decal_lock[0]->h + 2;
        decal_shoot[0] = gkrellm_create_decal_text(panel[0], "Shoot", ts_alt, style, x, 2, 0);

        decal_lock[1] = gkrellm_create_decal_text(panel[1], "L o c k", ts, style, 7, 2, 0);

        decal_shoot[1] = gkrellm_create_decal_text(panel[2], "S h o o t", ts, style, 2, 2, 0);


	/* Configure the panel to hold the above created decals, add in a little
	|  bottom margin for looks, and create the panel.
	*/
        for ( i=0; i < 3; i++ )
        {
           gkrellm_panel_configure(panel[i], NULL, style);
           gkrellm_panel_create(vbox, mon, panel[i] );
        }

        /* After the panel is created, the decals can be converted into buttons.
        |  First draw the initial text into the text decal button and then
        |  put the text decal into a meter button.  
        */
        gkrellm_draw_decal_text(panel[0], decal_lock[0], "Lock", 0 ); 
        gkrellm_put_decal_in_meter_button(panel[0], decal_lock[0], cb_button,
                                GINT_TO_POINTER(0), NULL);

        gkrellm_draw_decal_text(panel[0], decal_shoot[0], "Shoot", 0 ); 
        gkrellm_put_decal_in_meter_button(panel[0], decal_shoot[0], cb_button,
                                GINT_TO_POINTER(1), NULL);

        gkrellm_draw_decal_text(panel[1], decal_lock[1], "L o c k", 0 ); 
        gkrellm_put_decal_in_meter_button(panel[1], decal_lock[1], cb_button,
                                GINT_TO_POINTER(0), NULL);

        gkrellm_draw_decal_text(panel[2], decal_shoot[1], "S h o o t", 0 ); 
        gkrellm_put_decal_in_meter_button(panel[2], decal_shoot[1], cb_button,
                                GINT_TO_POINTER(1), NULL);

	/* Note: all of the above gkrellm_draw_decal_XXX() calls will not
	|  appear on the panel until a 	gkrellm_draw_layers(panel); call is
	|  made.  This will be done here because we don't have 
	|  update_plugin().
	*/

        for ( i=0; i < 3; i++ )
        {
          gkrellm_draw_panel_layers(panel[i]);
        }
        show_lock_shoot_select();

        if (shoot_tips == NULL)
        {
          shoot_tips = gtk_tooltips_new();
          shoot_tips_text = g_strdup("Click <Shoot> to grab window or screen\nClick <Lock> to lock the screen");
          gtk_tooltips_set_tip(shoot_tips, panel[0]->drawing_area,
                               shoot_tips_text, NULL);
          gtk_tooltips_set_delay(shoot_tips, 1000);
        }

    tmp_w = gkrellm_chart_width();
    if ( chart_w != tmp_w )
    {   
       chart_w = tmp_w;
       for ( i = 0; i < MAX_PANELS; i++ )
           {
             rgbbuf_t[i] = g_renew( guchar, rgbbuf_t[i],chart_w * CHART_H * 3 );
             blank_buf(i);
           }     
    }

	if (first_create)
	{	
        for ( i = 0; i < 3; i++ )
        {
	    g_signal_connect(GTK_OBJECT (panel[i]->drawing_area), "expose_event",
    	         G_CALLBACK( expose_event ), GINT_TO_POINTER(i));

	    g_signal_connect(GTK_OBJECT (panel[i]->drawing_area), 
	 	         "button_press_event",
    	         G_CALLBACK( panel_press ), NULL);
        }

        for ( i = 0; i < MAX_PANELS; i++ )
		{
	      g_signal_connect(GTK_OBJECT (chart[i]->drawing_area), "expose_event",
    	        G_CALLBACK ( chart_expose_event ), GINT_TO_POINTER(i) );
	      g_signal_connect(GTK_OBJECT (chart[i]->drawing_area), 
	        "button_press_event",
    	        G_CALLBACK ( anim_chart_press ), GINT_TO_POINTER(i));
		}

	    gdk_rgb_init();

            for ( i = 0; i < MAX_PANELS; i++ )
	    {
	      blank_buf( i );
              gkrellm_chart_enable_visibility(chart[i], i < active_panels , 
                                      &panel_visible[i] );
            }
	}  

}

#define PLUGIN_CONFIG_KEYWORD    "gkrellshoot"

static void
save_shoot_config (FILE *f)
{
    gint i;

    fprintf(f, "%s xlock_cmd %s\n", PLUGIN_CONFIG_KEYWORD,
            xlock_cmd);
    fprintf(f, "%s active_panels %d\n", PLUGIN_CONFIG_KEYWORD,
            active_panels);
    fprintf(f, "%s window_or_full %d\n", PLUGIN_CONFIG_KEYWORD,
            window_or_full);
    fprintf(f, "%s view_image %d\n", PLUGIN_CONFIG_KEYWORD,
            view_image);
    fprintf(f, "%s wait_seconds %d\n", PLUGIN_CONFIG_KEYWORD,
            wait_seconds);
    fprintf(f, "%s view_cmd %s\n", PLUGIN_CONFIG_KEYWORD, view_cmd );
    fprintf(f, "%s image_format %s\n", PLUGIN_CONFIG_KEYWORD, image_format );
    fprintf(f, "%s with_frame %d\n", PLUGIN_CONFIG_KEYWORD,
            with_frame);
    fprintf(f, "%s grayscale %d\n", PLUGIN_CONFIG_KEYWORD, grayscale );
    fprintf(f, "%s save_dir %s\n", PLUGIN_CONFIG_KEYWORD, save_dir);
    fprintf(f, "%s ff_select %s\n", PLUGIN_CONFIG_KEYWORD, ff_select);
    fprintf(f, "%s lock_shoot_select %d\n", PLUGIN_CONFIG_KEYWORD,
            lock_shoot_select);

	for ( i=0; i < MAX_PANELS; i++ )
	{
      fprintf(f, "%s anim_select%d %s\n", PLUGIN_CONFIG_KEYWORD, 
	          i,anim_select[i] );
      fprintf(f, "%s cycle_anim%d %d\n", PLUGIN_CONFIG_KEYWORD, 
	          i,cycle_anim[i] );
	}
}

static gboolean
valid_anim_type( gchar *value, gint i )
{
  gint j;

  gboolean result = FALSE;

  for ( j =0; j < MAX_ANIM; j++ )
  {
    if ( ! strcmp( value, anim_name[j] ) ) 
    { 
      current_anim[i] = j;	  
      result = TRUE; 
	  break;
    }
  }
  return result;
}
static void
load_shoot_config (gchar *arg)
{
    gchar config[64], item[1024];
	gchar tmp_buf[64];
    gint n, i;

    n = sscanf(arg, "%s %[^\n]", config, item);
    if (n == 2)
    {
        if (strcmp(config, "xlock_cmd") == 0)
	    {	
            strcpy(xlock_cmd, item);
	    }    
        if (strcmp(config, "active_panels") == 0)
            sscanf(item, "%d\n", &(active_panels));
        if (strcmp(config, "window_or_full") == 0)
            sscanf(item, "%d\n", &(window_or_full));
        if (strcmp(config, "view_image") == 0)
            sscanf(item, "%d\n", &(view_image));
        if (strcmp(config, "wait_seconds") == 0)
            sscanf(item, "%d\n", &(wait_seconds));
        if (strcmp(config, "view_cmd") == 0)
	    {	
            strcpy(view_cmd, item);
	    }    
        if (strcmp(config, "image_format") == 0)
	    {	
            strcpy(image_format, item);
	    }    

		for ( i = 0; i < MAX_PANELS; i++ )
		{
		  sprintf( tmp_buf, "anim_select%d",i);
          if (strcmp(config, tmp_buf) == 0)
	      {	
	        if ( valid_anim_type( item, i ) ) { strcpy(anim_select[i], item); }
	      }    
		  sprintf( tmp_buf, "cycle_anim%d",i);
          if (strcmp(config, tmp_buf) == 0)
	      {	
             sscanf(item, "%d\n", &(cycle_anim[i]));
	      }    
		}  
        if (strcmp(config, "with_frame") == 0)
            sscanf(item, "%d\n", &(with_frame));
 	if (strcmp(config, "grayscale") == 0)
 	    sscanf(item, "%d\n", &(grayscale));
        if (strcmp(config, "save_dir") == 0) {	strcpy(save_dir, item); }    

        if (strcmp(config, "ff_select") == 0) {	strcpy(ff_select, item); }    

        if (strcmp(config, "lock_shoot_select") == 0)
            sscanf(item, "%d\n", &(lock_shoot_select));
    }

}


static void
apply_shoot_config (void)
{
    gchar *c;
    const gchar *c_text;

	gint i;

    active_panels = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(num_panel_option));

    with_frame = GTK_TOGGLE_BUTTON(frame_option)->active;

    grayscale = GTK_TOGGLE_BUTTON(grayscale_option)->active;

    window_or_full = GTK_TOGGLE_BUTTON(window_option)->active;

    view_image = GTK_TOGGLE_BUTTON(view_image_option)->active;


   for ( i=0; i < active_panels; i++ )
   {
     c = gkrellm_gtk_entry_get_text(&(GTK_COMBO(anim_select_option[i])->entry));
     if (strcmp(anim_select[i], c) && valid_anim_type( c, i ) ) {
	  strcpy( anim_select[i], c );

	  blank_buf( i );
    }
    cycle_anim[i] = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(cycle_anim_option[i]));
   }

    c_text = gtk_entry_get_text(GTK_ENTRY(xlock_cmd_option));
    if (strcmp(xlock_cmd, c_text)) {
	  strcpy( xlock_cmd, c_text );
    }

    c_text = gtk_entry_get_text(GTK_ENTRY(view_cmd_option));
    if (strcmp(view_cmd, c_text)) {
	strcpy( view_cmd, c_text );    
    }

    c_text = gtk_entry_get_text(GTK_ENTRY(image_format_option));
    if (strcmp(image_format, c_text)) {
	strcpy( image_format, c_text );    
    }

    wait_seconds = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(wait_seconds_option));

    c_text = gtk_entry_get_text(GTK_ENTRY(save_dir_option));

    if ( g_file_test(c_text, G_FILE_TEST_IS_DIR ) )
    {
      if (strcmp(save_dir, c_text)) {
  	  strcpy( save_dir, c_text );
      }
    }

    c = gkrellm_gtk_entry_get_text(&(GTK_COMBO(ff_select_option)->entry));
     if (strcmp(ff_select, c) ) { strcpy( ff_select, c ); } 

}
static GtkWidget *create_anim_config_tab( gint i )
{
    GtkWidget *vbox, *hbox, *label;
    GList *items = NULL;
    GtkAdjustment *cycle_anim_adjust;

    gint j;

	vbox = gtk_vbox_new( FALSE, 0 );
	hbox = gtk_hbox_new( FALSE, 0 );


    label = gtk_label_new("Select Animation " );

    for ( j = 0; j < MAX_ANIM; j++ )
	{
      items = g_list_append (items, anim_name[j] );
	}

    anim_select_option[i] = gtk_combo_new();
    gtk_combo_set_popdown_strings (GTK_COMBO (anim_select_option[i]), items);
    gtk_combo_set_value_in_list( GTK_COMBO(anim_select_option[i]), TRUE, FALSE );
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(anim_select_option[i])->entry),
                                                anim_select[i] );

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), anim_select_option[i], FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

	hbox = gtk_hbox_new( FALSE, 0 );
    label = gtk_label_new("Cycle through Animation every " );
    gtk_box_pack_start(GTK_BOX(hbox), label , FALSE, FALSE, 0);

    cycle_anim_adjust = (GtkAdjustment *) gtk_adjustment_new((gfloat)
            cycle_anim[i], 0, 60.0, 1.0, 5.0, 0.0);
    cycle_anim_option[i] = gtk_spin_button_new(cycle_anim_adjust, 1.0, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(cycle_anim_option[i]),
            (guint) 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(cycle_anim_option[i]),
            cycle_anim[i]);

    gtk_box_pack_start(GTK_BOX(hbox), cycle_anim_option[i], FALSE, FALSE, 0);

    label = gtk_label_new(" minutes (0 means animation will not cycle)" );
    gtk_box_pack_start(GTK_BOX(hbox), label , FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

    gtk_widget_show_all( vbox );

	return vbox;
}

static void insert_anim_config_tab (int i)
{
  GtkWidget *label, *configpanel;
  gchar *labeltxt;
  
  if (!GTK_IS_OBJECT ( laptop ))
    return;

  configpanel = create_anim_config_tab (i);
  
  labeltxt = g_strdup_printf (_("Animation#%i"), i + 1);
  label = gtk_label_new (labeltxt);
  g_free (labeltxt);
  
  gtk_notebook_insert_page (GTK_NOTEBOOK ( laptop ), configpanel, label, i + 1);
}


static void remove_anim_config_tab( gint i )
{
  if (!GTK_IS_OBJECT (laptop)) { return; }

  gtk_notebook_remove_page (GTK_NOTEBOOK (laptop), i + 1);
 
}
static void change_num_panels()
{
  gint i;

  if ( active_panels == sel_num_panels ) { return; }

  for ( i = active_panels - 1; i >= sel_num_panels; i-- )
  {
    remove_anim_config_tab( i );
  }
  for ( i = 0; i < MAX_PANELS; i++ )
  {
     blank_buf( i );
     gkrellm_chart_enable_visibility(chart[i], i < sel_num_panels , 
                                      &panel_visible[i] );
  }							 

  for ( i = active_panels; i < sel_num_panels; i++ )
  {
    insert_anim_config_tab( i );
  }

  active_panels = sel_num_panels;

}
static void num_panel_changed()
{
  sel_num_panels = gtk_spin_button_get_value_as_int
                              (GTK_SPIN_BUTTON (num_panel_option));

  if ( sel_num_panels < MIN_PANELS || sel_num_panels > MAX_PANELS )
  {
    sel_num_panels = MIN_PANELS;
  }
  change_num_panels ();
}

static void
cb_lock_shoot_select(GtkWidget *button, gpointer data)
{
        gint    i = GPOINTER_TO_INT(data);

        if (GTK_TOGGLE_BUTTON(button)->active)
                lock_shoot_select = i;

        show_lock_shoot_select();
}

static void
create_shoot_tab (GtkWidget *tab)
{
    GtkWidget *frame, *ybox, *hbox, *vbox, *separator, 
              *label, *text, *about_label;
    GtkAdjustment *wait_adjust, *num_panel_adjust;

    GtkWidget *tablabel, *anim_config, *button;
	gchar *anim_tab_name;

    GList *items = NULL;

    gint i, j;


    gchar *about_text = NULL;
    static gchar *help_text[] =
    {
      N_("<h>" CONFIG_NAME " " GKRELLSHOOT_VER "\n\n" ),
      N_("Grabs screen/window when Shoot is clicked (uses ImageMagick 5.5.7 or greater) \n" ),
      N_("Locks the screen when Lock is clicked. And optionally will display " ),
      N_("Animation. \n\n" ),
      N_("<b> Options \n\n" ),
      N_("Save Location - valid path to save images when shoot is clicked.\n"),
      N_("Date Format - select data format to append to filename .\n"),
      N_("Lock/Shoot Select - choose the button to be displayed .\n"),
      N_("Image Format - any valid image format supported by ImageMagick.\n"),
      N_("Maximum of 3 animation panels are possible.Each can have its\n"),
      N_("own animation. To Disable animation choose 0.\n\n"),
      N_("<b> Animation Panel \n\n" ),
      N_("Select Animation from dropdown. To cycle through animation choose\n"),
      N_("number of minutes between cycle.\n"),
      N_("Middle-click the Animation to cycle through all available animations.\n"),
      N_("Right-click to open config window .\n\n"),
      N_("<b> Usage \n\n" ),
      N_("Once Shoot is clicked if window grab is selected then cross prompt \n"),
      N_("appears on the screen using which one can select a window\n"),
      N_("by clicking on a window or hold down mouse and choose an area \n"),
      N_("to be grabbed, otherwise whole screen is grabbed. \n\n "),
      N_("If with window frame is selected, windows are grabbed with their frame. \n\n"),
      N_("If grayscale is selected you get grayscaled image. \n\n"),
      N_("Grabbed image is saved on to the directory specified in Save Location with the\n"),
      N_("name gkrellShoot_<Date Format Option>_HHMMSS.imageFormat(jpg,gif etc). \n\n"),
      N_("If View Image is selected Grabbed Image is passed onto given "),
      N_("Image Viewer. ")
    };

	if ( laptop ) { gtk_object_unref( GTK_OBJECT( laptop ) ); }

    laptop = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(laptop), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(tab), laptop, TRUE, TRUE, 0);
    gtk_object_ref (GTK_OBJECT (laptop));
    /* options */
    frame = gtk_frame_new(NULL);
    gtk_container_border_width(GTK_CONTAINER(frame), 3);

    vbox = gtk_vbox_new( FALSE, 0 );
    gtk_container_border_width(GTK_CONTAINER(vbox), 3);

    /* Lock */
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Screen Lock Command" );
    xlock_cmd_option = gtk_entry_new_with_max_length(512);
    gtk_entry_set_text(GTK_ENTRY(xlock_cmd_option), xlock_cmd);
    gtk_entry_set_editable(GTK_ENTRY(xlock_cmd_option), TRUE);
    
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), xlock_cmd_option, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);

    /* SaveDir */
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Save Location " );
    save_dir_option = gtk_entry_new_with_max_length(512);
    gtk_entry_set_text(GTK_ENTRY(save_dir_option), save_dir);
    gtk_entry_set_editable(GTK_ENTRY(save_dir_option), TRUE);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), save_dir_option, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);


    /* Date Format to append to filename */
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Date Format( to append to filename) " );

    for ( j = 0; j < MAX_F_FORMAT; j++ )
    {
      items = g_list_append (items, ff_name[j] );
    }
    ff_select_option = gtk_combo_new();
    gtk_combo_set_popdown_strings (GTK_COMBO (ff_select_option), items);
    gtk_combo_set_value_in_list( GTK_COMBO(ff_select_option), TRUE, FALSE );
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ff_select_option)->entry), ff_select );

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ff_select_option, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);

    /* Lock_Shoot Select */
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 4);

     ybox = gkrellm_gtk_framed_vbox(vbox, _("Lock/Shoot Select"),
                        4, FALSE, 0, 2);
        
     hbox = gtk_hbox_new(FALSE, 0);
     gtk_box_pack_start(GTK_BOX(ybox), hbox, FALSE, FALSE, 0);

     lock_shoot_option[0] = gtk_radio_button_new_with_label(NULL, _("Both"));
     gtk_box_pack_start(GTK_BOX(hbox), lock_shoot_option[0], TRUE, TRUE, 0);
     lock_shoot_option[1] = gtk_radio_button_new_with_label_from_widget(
                                        GTK_RADIO_BUTTON(lock_shoot_option[0]), _("Lock"));
     gtk_box_pack_start(GTK_BOX(hbox), lock_shoot_option[1], TRUE, TRUE, 0);
     lock_shoot_option[2] = gtk_radio_button_new_with_label_from_widget(
                                        GTK_RADIO_BUTTON(lock_shoot_option[1]), _("Shoot"));
     gtk_box_pack_start(GTK_BOX(hbox), lock_shoot_option[2], TRUE, TRUE, 0);

     button = lock_shoot_option[lock_shoot_select];
     gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
        
     for (i = 0; i < 3; ++i) {
                g_signal_connect(G_OBJECT(lock_shoot_option[i]), "toggled",
                                        G_CALLBACK(cb_lock_shoot_select), GINT_TO_POINTER(i));
     }

    /* Number of Animation */
    hbox = gtk_hbox_new(FALSE, 0);

    num_panel_adjust = (GtkAdjustment *) gtk_adjustment_new (
	                         (gfloat) active_panels,
                             (gfloat) MIN_PANELS,
                             (gfloat) MAX_PANELS,
                             1.0, 1.0, 0);
    num_panel_option = gtk_spin_button_new (num_panel_adjust, 1.0, 0);
    gtk_signal_connect (GTK_OBJECT (num_panel_option), "changed",
                      GTK_SIGNAL_FUNC (num_panel_changed), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), num_panel_option, FALSE, FALSE, 0);

    label = gtk_label_new("Number of Animation Panels ( To disable animation choose 0 ) " );
    gtk_box_pack_start(GTK_BOX(hbox), label , FALSE, FALSE, 5);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);

    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 4);
    /* Shoot */
    label = gtk_label_new("Screen Shoot Options");
    gtk_container_add(GTK_CONTAINER(vbox), label);

    window_option = gtk_check_button_new_with_label( WINDOW_LABEL );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window_option),
            window_or_full );

    gtk_container_add(GTK_CONTAINER(vbox), window_option);

    frame_option = gtk_check_button_new_with_label( "with window frame" );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(frame_option),
            with_frame );
    gtk_container_add(GTK_CONTAINER(vbox), frame_option);

    grayscale_option = gtk_check_button_new_with_label( "grayscale" );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grayscale_option),
            grayscale );
    gtk_container_add(GTK_CONTAINER(vbox), grayscale_option);

    view_image_option = gtk_check_button_new_with_label( "View Image after click" );
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view_image_option),
            view_image );
    gtk_container_add(GTK_CONTAINER(vbox), view_image_option);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Image Viewer " );
    view_cmd_option = gtk_entry_new_with_max_length(512);
    gtk_entry_set_text(GTK_ENTRY(view_cmd_option), view_cmd);
    gtk_entry_set_editable(GTK_ENTRY(view_cmd_option), TRUE);
    
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), view_cmd_option, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);

    ybox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Seconds to wait before Screen Shot");
    gtk_box_pack_start(GTK_BOX(ybox), label, FALSE, FALSE, 0);
    wait_adjust = (GtkAdjustment *) gtk_adjustment_new((gfloat)
            wait_seconds, 0, 180.0, 1.0, 5.0, 0.0);
    wait_seconds_option = gtk_spin_button_new(wait_adjust, 1.0, 1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(wait_seconds_option),
            (guint) 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(wait_seconds_option),
            wait_seconds);
    gtk_box_pack_start(GTK_BOX(ybox), wait_seconds_option, FALSE, FALSE, 0);

    label = gtk_label_new(" Image Format" );
    image_format_option = gtk_entry_new_with_max_length(8);
    gtk_entry_set_text(GTK_ENTRY(image_format_option), image_format);
    gtk_entry_set_editable(GTK_ENTRY(image_format_option), TRUE);
    
    gtk_box_pack_end(GTK_BOX(ybox), image_format_option, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(ybox), label, FALSE, FALSE, 0);


    gtk_container_add(GTK_CONTAINER(vbox), ybox);

    label = gtk_label_new("Options");
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_notebook_append_page(GTK_NOTEBOOK(laptop), frame, label);

    /* Individual Panel options tabs */
	for (i = 0; i < MAX_PANELS; i++)
    {
      anim_config = create_anim_config_tab (i);

      anim_tab_name = g_strdup_printf (_("Animation#%d"), i + 1 );
      tablabel = gtk_label_new (anim_tab_name);
      g_free (anim_tab_name);
    
      if (i < active_panels)
	  {
        gtk_notebook_append_page (GTK_NOTEBOOK (laptop), anim_config, tablabel);
	  }	
    }
        /* help */
	vbox = gkrellm_gtk_framed_notebook_page(laptop, _("Help"));
	text = gkrellm_gtk_scrolled_text_view(vbox, NULL,
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	for (i = 0; i < sizeof(help_text)/sizeof(gchar *); ++i)
		gkrellm_gtk_text_view_append(text, _(help_text[i]));

        /* about */
    about_text = g_strdup_printf(
        "GKrellShoot %s\n" \
        "GKrellM Shoot Plugin\n" \
        "\n" \
        "Copyright (C) %s M.R.Muthu Kumar\n" \
        "m_muthukumar@users.sourceforge.net\n" \
        "\n" \
        "Released under the GNU Public License\n" \
	"GkrellShoot comes with ABSOLUTELY NO WARRANTY\n" \
        , GKRELLSHOOT_VER, GKRELLSHOOT_YEAR
    );
    about_label = gtk_label_new(about_text);
    g_free(about_text);
    label = gtk_label_new("About");
    gtk_notebook_append_page(GTK_NOTEBOOK(laptop), about_label, label);
}


/* The monitor structure tells GKrellM how to call the plugin routines.
*/
static GkrellmMonitor	plugin_mon	=
	{
	CONFIG_NAME,        	/* Name, for config tab.    */
	0,			/* Id,  0 if a plugin       */
	create_plugin,		/* The create function      */
	update_plugin,			/* The update function      */
	create_shoot_tab,	/* The config tab create function   */
	apply_shoot_config,	/* Apply the config function        */ 
	save_shoot_config,	/* Save user config*/
	load_shoot_config,	/* Load user config*/
	PLUGIN_CONFIG_KEYWORD,	/* config keyword*/ 
	NULL,			/* Undefined 2	*/
	NULL,			/* Undefined 1	*/
	NULL,			/* private		*/ 
	MON_UPTIME,		/* Insert plugin before this monitor*/ 
	NULL,			/* Handle if a plugin, filled in by GKrellM */
	NULL			/* path if a plugin, filled in by GKrellM   */
	};

static void
read_default(void)
{
   gint i;

   wait_seconds = 0;
   window_or_full = 1;
   view_image = 1;
   active_panels = 1;
   sel_num_panels = 1;
   
   chart_w = gkrellm_chart_width();

   for ( i = 0; i < MAX_PANELS; i++ )
   {
     panel_visible[i] = TRUE;
	 cycle_anim[i] = 0;
     current_anim[i] = i + 1;
	 if ( i >= MAX_ANIM) { current_anim[i] = 0; }
     sprintf( anim_select[i],"%s",  anim_name[current_anim[i]] );
	 rgbbuf_t[i] = g_new0( guchar, chart_w * CHART_H * 3 );

   }

   sprintf( xlock_cmd,"%s",  DEFAULT_XLOCK );
   sprintf( view_cmd,"%s",  DEFAULT_VIEW );
   sprintf( image_format,"%s",  DEFAULT_IMAGE );


   sprintf( save_dir,"%s", gkrellm_homedir() );

   sprintf( filename, "%s/%s", save_dir, DEFAULT_OUTFILE );

   sprintf( ff_select,"%s",  MMDDYY );

   gk_ticks = gkrellm_ticks();
}	


  /* All GKrellM plugins must have one global routine named init_plugin()
  |  which returns a pointer to a filled in monitor structure.
  */
GkrellmMonitor *
gkrellm_init_plugin( void )
	{
	/* If this call is made, the background and krell images for this plugin
	|  can be custom themed by putting bg_meter.png or krell.png in the
	|  subdirectory STYLE_NAME of the theme directory.  Text colors (and
	|  other things) can also be specified for the plugin with gkrellmrc
	|  lines like:  StyleMeter  STYLE_NAME.textcolor orange black shadow
	|  If no custom themeing has been done, then all above calls using
	|  style_id will be equivalent to style_id = DEFAULT_STYLE_ID.
	*/
	style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME); 

	/* style_id = gkrellm_lookup_meter_style_id(CAL_STYLE_NAME); */


	read_default();

	mon = &plugin_mon;
	return &plugin_mon;
	}
