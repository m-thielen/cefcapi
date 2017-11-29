/******************************************************************************
 *
 * loader.c - win32 CEF loader app with its own Windows message loop.
 *
 * Based on https://github.com/cztomczak/cefcapi
 *
 * Author: Markus Thielen, mt@thiguten.de
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*--- includes --------------------------------------------------------------*/

#include <windows.h>
#include "capi/cef_base.h"
#include "capi/cef_app.h"
#include "capi/cef_client.h"

/*--- global data -----------------------------------------------------------*/

cef_life_span_handler_t g_life_span_handler = {0};

HWND                    hwnd_main;

HINSTANCE               hinst_main;

cef_browser_t           *g_browser = NULL;

/*--- function prototypes ---------------------------------------------------*/

HWND              create_main_window          (HINSTANCE inst);

LRESULT CALLBACK  main_wnd_proc               (HWND hwnd,
                                               UINT msg,
                                               WPARAM wp,
                                               LPARAM lp);

void              init_cef_window             (HWND hwnd);

void              init_cef_life_span_handler  (cef_life_span_handler_t* handler);

/*--- start of code ---------------------------------------------------------*/

/******************************************************************************
 * main
 */
int main(int argc, char**argv)
{
  MSG msg;
  int cef_exe_code;

  printf("\nProcess args: ");
  if (argc == 1) {
    printf("none (Main process)");
  } else {
    for (int i = 1; i < argc; i++) {
      if (strlen(argv[i]) > 128) {
        printf("... ");
      } else {
        printf("%s ", argv[i]);
      }
    }
  }
  printf("\n\n");

  hinst_main = GetModuleHandle(NULL);

  cef_app_t app = {0};
  initialize_cef_app(&app);

  /* application settings */
  cef_settings_t settings;
  memset(&settings, 0x00, sizeof(settings));
  settings.size = sizeof(cef_settings_t);
  settings.log_severity = LOGSEVERITY_WARNING; // Show only warnings/errors
  settings.no_sandbox = 1;
  settings.multi_threaded_message_loop = 1;

  cef_main_args_t main_args;
  memset(&main_args, 0x00, sizeof(main_args));
  main_args.instance = hinst_main;
  cef_initialize(&main_args, &settings, &app, NULL);

  /* Execute subprocesses. It is also possible to have
   * a separate executable for subprocesses by setting
   * cef_settings_t.browser_subprocess_path. In such
   * case cef_execute_process should not be called here.
   */
  printf("cef_execute_process\n");
  cef_exe_code = cef_execute_process(&main_args, &app, NULL);
  if (cef_exe_code >= 0) {
    /* subprocess is supposed to end here */
    return cef_exe_code;
  }

  /* cef_execute_process returns -1 for the host process, i.e. the process
   * that must create the main window and run the message loop
   */
  if (cef_exe_code == -1) {

    /* create main window */
    hwnd_main = create_main_window(hinst_main);
    printf("Main window handle 0x%08x\n", (unsigned int) hwnd_main);

    /* run message loop */
    while(GetMessage (&msg, NULL, 0, 0))  {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }

  }

  cef_shutdown();

  return 0;
}

/******************************************************************************
 * create_main_window - create the window that hosts the chromium control
 */
HWND create_main_window(HINSTANCE inst)
{
  WNDCLASSEX wcex;
  HWND hwnd;
  const char *class_name = "main-window-class";

  memset(&wcex, 0x00, sizeof(wcex));
  wcex.cbSize        = sizeof (wcex);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = main_wnd_proc;
  wcex.hInstance     = inst;
  wcex.hCursor       = LoadCursor (NULL, IDC_ARROW);
  wcex.hbrBackground = WHITE_BRUSH;
  wcex.lpszClassName = class_name;
  RegisterClassEx (&wcex);
  hwnd = CreateWindow(class_name, "Win32 App Loader",
                      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0,
                      CW_USEDEFAULT, 0, NULL, NULL, inst, NULL);
  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);
  return hwnd;
}

/******************************************************************************
 * init_cef_window() - initialize the CEF window as a control window
 * in our main window's client area
 */
void init_cef_window(HWND hwnd)
{
  RECT rc;
  cef_window_info_t wi;

  /* get size of window client area */
  memset(&rc, 0x00, sizeof(rc));
  GetClientRect(hwnd, &rc);

  /* initialize CEF window info so the CEF window becomes a child window
   * displayed in our main window's client area */
  memset(&wi, 0x00, sizeof(wi));
  wi.style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VISIBLE;
  wi.parent_window = hwnd;
  wi.x = rc.left;
  wi.y = rc.top;
  wi.width = rc.right - rc.left;
  wi.height = rc.bottom - rc.top;
  wi.transparent_painting_enabled = 1;
  /* window title */
  char title[] = "Win32 CEF Loader";
  cef_string_t cef_title = {0};
  cef_string_utf8_to_utf16(title, strlen(title), &cef_title);
  wi.window_name = cef_title;

  /* URL, hard coded for now */
  char url[] = "https://www.thiguten.de/transparent.html";
  cef_string_t cef_url = {0};
  cef_string_utf8_to_utf16(url, strlen(url), &cef_url);

  /* browser settings */
  cef_browser_settings_t browser_settings;
  memset(&browser_settings, 0x00, sizeof(browser_settings));
  browser_settings.size = sizeof(cef_browser_settings_t);

  /* client handlers */
  cef_client_t client;
  memset(&client, 0x00, sizeof(client));
  initialize_cef_client(&client);
  init_cef_life_span_handler(&g_life_span_handler);

  /* create browser */
  printf("cef_browser_host_create_browser\n");
  cef_browser_host_create_browser(&wi, &client, &cef_url,
                                  &browser_settings, NULL);

  /* make main window transparent */
  SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd_main, GWL_EXSTYLE) | WS_EX_LAYERED);
  SetLayeredWindowAttributes(hwnd, RGB(255,255,255), 0, LWA_COLORKEY);
}


/******************************************************************************
 * main window procedure
 */
LRESULT CALLBACK main_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  switch(msg) {

    case WM_CREATE:
      init_cef_window(hwnd_main);
      return 0;

    case WM_SIZE: {
      // from the cefclient example, do not allow the window to be resized to 0x0 or the layout will break;
      // also be aware that if the size gets too small, GPU acceleration disables
      if ((wp != SIZE_MINIMIZED) && g_browser) {
        cef_browser_host_t *host = g_browser->get_host(g_browser);
        HWND cef_hwnd = host->get_window_handle(host);
        if (cef_hwnd) {
          RECT rect = { 0 };
          GetClientRect (hwnd, &rect);
          HDWP hdwp = BeginDeferWindowPos (1);
          hdwp = DeferWindowPos (hdwp, cef_hwnd, NULL,rect.left,
          rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
          EndDeferWindowPos (hdwp);
        }
      }
    }
    break;

    case WM_ERASEBKGND: {
      if (g_browser) {
        cef_browser_host_t *host = g_browser->get_host(g_browser);
        HWND cef_hwnd = host->get_window_handle(host);
        if (cef_hwnd) {
          // from the cefclient example, don't erase the background
          // if the browser window has been loaded to avoid flashing
          return 1;
        }
      }
    }
    break;

    default:
      break;
  }

  return DefWindowProc(hwnd, msg, wp, lp);
}

/*---- CEF lifespan handler -------------------------------------------------*/

///
// Called after a new browser is created. This callback will be the first
// notification that references |browser|.
///
void CEF_CALLBACK on_after_created(struct _cef_life_span_handler_t* self,
                                   struct _cef_browser_t* browser)
{
  /* set our global browser object */
  g_browser = browser;
}

///
// Called just before a browser is destroyed. Release all references to the
// browser object and do not attempt to execute any functions on the browser
// object after this callback returns. This callback will be the last
// notification that references |browser|. See do_close() documentation for
// additional usage information.
///
void CEF_CALLBACK on_before_close(struct _cef_life_span_handler_t* self,
                                  struct _cef_browser_t* browser)
{
  DEBUG_CALLBACK("on_before_close\n");
  // TODO: Check how many browsers do exist and quit message
  //       loop only when last browser is closed. Otherwise
  //       closing a popup window will exit app while main
  //       window shouldn't be closed.
  cef_quit_message_loop();
}


void init_cef_life_span_handler(cef_life_span_handler_t* handler)
{
  DEBUG_CALLBACK("initialize_cef_life_span_handler\n");
  handler->base.size = sizeof(cef_life_span_handler_t);
  initialize_cef_base_ref_counted((cef_base_ref_counted_t*)handler);
  // callbacks - there are many, but implementing only one
  handler->on_before_close = on_before_close;
  handler->on_after_created = on_after_created;
}

