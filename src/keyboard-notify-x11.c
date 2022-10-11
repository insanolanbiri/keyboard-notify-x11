#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include <X11/XKBlib.h>
#include <libnotify/notify.h>

#include "notify_icons.h"

#define SLEEP_US(us) nanosleep((const struct timespec[]){{us / 1000000ul, (us % 1000000ul) * 1000}}, NULL)
#define SLEEP 80 * 1000  /* kaç mikrosaniye duracağı (1 ms = 1000 us) */
#define NOTIFY_SLEEP 800 /* bildirimin kaç milisaniye duracağı */
#define PRINT 0          /* print edilip edimeyeceği */
#define SIGNAL_HANDLING 1

static volatile unsigned char keepRunning = 1;

#if SIGNAL_HANDLING
struct sigaction siga;

void signalhandler(int signal)
{
#if PRINT
  printf("rcvd signal %d\n", signal);
#endif
  keepRunning = 0;
}
#endif // SIGNAL_HANDLING

int main()
{
#if SIGNAL_HANDLING
  siga.sa_handler = signalhandler;
  for (int sig = 1; sig <= SIGRTMAX; ++sig)
    sigaction(sig, &siga, NULL);
#endif // SIGNAL_HANDLING

  Display *display;

  unsigned int oldstate = 2 /* numlock acik capslock kapali */, state;

  char caps_degisiklik, num_degisiklik;

  char *message;
  GdkPixbuf *icon;
  GdkPixbuf *icon_caps_on = gdk_pixbuf_new_from_inline(sizeof(raw_caps_on), raw_caps_on, FALSE, NULL);
  GdkPixbuf *icon_caps_off = gdk_pixbuf_new_from_inline(sizeof(raw_caps_off), raw_caps_off, FALSE, NULL);
  GdkPixbuf *icon_num_on = gdk_pixbuf_new_from_inline(sizeof(raw_num_on), raw_num_on, FALSE, NULL);
  GdkPixbuf *icon_num_off = gdk_pixbuf_new_from_inline(sizeof(raw_num_off), raw_num_off, FALSE, NULL);

  display = XOpenDisplay(getenv("DISPLAY"));
  if (!display)
  {
    fprintf(stderr, "e hani x11 displayi bulamadım? BANA DISPLAYIMI VER\n");
    return 1;
  }

  notify_init("klavye tuş kilidi");
  NotifyNotification *n = notify_notification_new(NULL, NULL, NULL);
  notify_notification_set_timeout(n, NOTIFY_SLEEP);
  notify_notification_set_urgency(n, NOTIFY_URGENCY_NORMAL);

  while (keepRunning)
  {
    SLEEP_US(SLEEP);
    if (XkbGetIndicatorState(display, XkbUseCoreKbd, &state) != Success)
    {
      fprintf(stderr, "tuş durumları alınamadı\n");
      return 1;
    }

    if (state != oldstate)
    {
      caps_degisiklik = (state & 1) - (oldstate & 1);
      num_degisiklik = (state & 2) - (oldstate & 2);
#if PRINT
      printf("bişey değişti:%d->%d\n", oldstate, state);
      printf("c:%d,n:%d\n", caps_degisiklik, num_degisiklik);
#endif

      switch (caps_degisiklik)
      {
      case 1: /* kapali -> acik */
        message = "caps lock açık";
        icon = icon_caps_on;
        break;
      case -1: /* acik -> kapali */
        message = "caps lock kapalı";
        icon = icon_caps_off;
        break;
      default:
        break;
      }

      switch (num_degisiklik)
      {
      case 2: /* kapali -> acik */
        message = "num lock açık";
        icon = icon_num_on;
        break;
      case -2: /* acik -> kapali */
        message = "num lock kapalı";
        icon = icon_num_off;
        break;
      default:
        break;
      }

#if PRINT
      printf("%s\n", message);
#endif

      notify_notification_update(n, message, "", NULL);
      notify_notification_set_icon_from_pixbuf(n, icon);

      if (!notify_notification_show(n, 0))
      {
        fprintf(stderr, "bildirim gittirilemedi\n");
        return 1;
      }
      oldstate = state;
    }
  }
  notify_notification_close(n, NULL);
  return 0;
}
