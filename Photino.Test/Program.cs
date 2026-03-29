using System.ComponentModel;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using Photino.NET;
using Photino.NET.Utils;

namespace Photino.Test
{
    class Program
    {
        private static readonly bool s_logEvents = true;
        private static int s_windowNumber = 1;

        private static PhotinoWindow? s_mainWindow;

        [STAThread]
        static void Main(string[] args)
        {
            try
            {
                FluentStyle();
                //PropertyInitStyle();
            }
            catch (Exception ex)
            {
                Log(null, ex.Message);
                Console.ReadKey();
            }
        }

        private static void FluentStyle()
        {
            var iconFile = Platform.IsWindows
                ? "wwwroot/photino-logo.ico"
                : "wwwroot/photino-logo.png";

            //string browserInit = string.Empty;

            //if (PhotinoWindow.IsWindowsPlatform)
            //{
            //    //Windows example for WebView2
            //    browserInit = "--disable-web-security --hide-scrollbars ";
            //}
            //else if (PhotinoWindow.IsMacOsPlatform)
            //{
            //    //Mac example for Webkit on Cocoa
            //    browserInit = JsonSerializer.Serialize(new
            //    {
            //        setLegacyEncryptedMediaAPIEnabled = true
            //    });
            //}
            //else if (PhotinoWindow.IsLinuxPlatform)
            //{
            //    //Linux example for Webkit2Gtk
            //    browserInit = JsonSerializer.Serialize(new
            //    {
            //        set_enable_encrypted_media = true,
            //        //set_default_font_size = 48,
            //        //set_enable_developer_extras = true,
            //        set_default_font_family = "monospace"
            //    });
            //}

            s_mainWindow = new PhotinoWindow()
                //.Load(new Uri("https://google.com"))
                //.Load("https://duckduckgo.com/?t=ffab&q=user+agent+&ia=answer")
                //.Load("https://localhost:8080/")
                .Load("wwwroot/main.html")
                //.Load("wwwroot/index.html")
                //.LoadRawString("<h1>Hello PhotinoX!</h1>")

                //Window settings
                //.SetIconFile(iconFile)
                //.SetTitle($"My PhotinoX Window {_windowNumber++}")
                //.SetChromeless(true)
                //.SetTransparent(true)
                //.SetFullScreen(true)
                //.SetMaximized(true)
                //.SetMaxSize(640, 480)
                //.SetMinimized(true)
                .SetMinHeight(480)
                .SetMinWidth(640)
                //.SetMinSize(320, 240)
                //.SetResizable(true)
                //.SetTopMost(true)
                //.SetUseOsDefaultLocation(false)
                //.SetUseOsDefaultSize(false)
                .Center()
                //.SetSize(new Size(800, 600))
                //.SetHeight(600)
                //.SetWidth(800)
                //.SetLocation(new Point(50, 50))
                //.SetTop(50)
                //.SetLeft(50)
                //.MoveTo(new Point(10, 10))
                //.MoveTo(20, 20)
                //.Offset(new Point(150, 150))
                //.Offset(250, 250)
                .SetNotificationRegistrationId("8FDF1B15-3408-47A6-8EF5-2B0676B76277")  //Replaces the window title when registering toast notifications
                .SetNotificationsEnabled(false)

                //Browser settings
                //.SetContextMenuEnabled(false)
                .SetDevToolsEnabled(true)
                //.SetGrantBrowserPermissions(false)
                //.SetZoom(150)

                //Browser startup flags
                //.SetBrowserControlInitParameters("--ignore-certificate-errors ")
                .SetUserAgent("Custom PhotinoX User Agent")
                //.SetMediaAutoplayEnabled(true)
                //.SetFileSystemAccessEnabled(true)
                //.SetWebSecurityEnabled(true)
                //.SetJavascriptClipboardAccessEnabled(true)
                //.SetMediaStreamEnabled(true)
                //.SetSmoothScrollingEnabled(true)
                //.SetTemporaryFilesPath(@"C:\Temp")
                //.SetIgnoreCertificateErrorsEnabled(false)

                .RegisterCustomSchemeHandler("app", AppCustomSchemeUsed)

                .RegisterWindowCreatingHandler(WindowCreating)
                .RegisterWindowCreatedHandler(WindowCreated)
                .RegisterLocationChangedHandler(WindowLocationChanged)
                .RegisterSizeChangedHandler(WindowSizeChanged)
                .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                .RegisterWindowClosingHandler(WindowClosing)
                .RegisterFocusInHandler(WindowFocusIn)
                .RegisterFocusOutHandler(WindowFocusOut)

                .SetLogVerbosity(s_logEvents ? 2 : 0);

            s_mainWindow.WaitForClose();

            Console.WriteLine("Done Blocking!");
        }

        private static void PropertyInitStyle()
        {
            var iconFile = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? "wwwroot/photino-logo.ico"
                : "wwwroot/photino-logo.png";

            var browserInit = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? "--disable-web-security --hide-scrollbars "           //Windows example for WebView2
                : RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                    ? "{ 'set_enable_encrypted_media': true }"          //Linux example for Webkit2Gtk
                    : "{ 'setLegacyEncryptedMediaAPIEnabled': true }";  //Mac example for Webkit on Cocoa

            s_mainWindow = new PhotinoWindow
            {
                //StartUrl = "https://google.com",
                //StartUrl = "https://duckduckgo.com/?t=ffab&q=user+agent+&ia=answer",
                StartUrl = "wwwroot/main.html",
                //StartString = "<h1>Hello PhotinoX!</h1>",

                //Window settings
                IconFile = iconFile,
                Title = $"My PhotinoX Window {s_windowNumber++}",
                //Chromeless = true,
                //Transparent = true,
                //FullScreen = true,
                //Maximized = true,
                MaxWidth = 640,
                MaxHeight = 480,
                //MaxSize = new Point(640, 480),
                //Minimized = true,
                MinWidth = 320,
                MinHeight = 240,
                //MinSize = new Point(320, 240),
                //Resizable = false,
                //TopMost = true,
                UseOsDefaultLocation = false,
                UseOsDefaultSize = false,
                Centered = true,
                Size = new Size(800, 600),
                Height = 600,
                Width = 800,
                Location = new Point(50, 50),
                Top = 50,
                Left = 50,
                NotificationRegistrationId = "8FDF1B15-3408-47A6-8EF5-2B0676B76277",  //Replaces the window title when registering toast notifications
                NotificationsEnabled = false,

                //Browser settings
                ContextMenuEnabled = false,
                DevToolsEnabled = false,
                GrantBrowserPermissions = false,
                Zoom = 150,

                //Browser startup flags
                BrowserControlInitParameters = browserInit,
                UserAgent = "Custom PhotinoX User Agent",
                MediaAutoplayEnabled = true,
                FileSystemAccessEnabled = true,
                WebSecurityEnabled = true,
                JavascriptClipboardAccessEnabled = true,
                MediaStreamEnabled = true,
                SmoothScrollingEnabled = true,
                //TemporaryFilesPath = @"C:\Temp",
                //IgnoreCertificateErrorsEnabled = false,

                LogVerbosity = s_logEvents ? 2 : 0,
            };

            s_mainWindow.WindowCreating += WindowCreating;
            s_mainWindow.WindowCreated += WindowCreated;
            s_mainWindow.WindowLocationChanged += WindowLocationChanged;
            s_mainWindow.WindowSizeChanged += WindowSizeChanged;
            s_mainWindow.WindowMaximized += WindowMaximized;
            s_mainWindow.WindowRestored += WindowRestored;
            s_mainWindow.WindowMinimized += WindowMinimized;
            s_mainWindow.WebMessageReceived += MessageReceivedFromWindow;
            s_mainWindow.WindowClosing += WindowClosing;
            s_mainWindow.WindowFocusIn += WindowFocusIn;
            s_mainWindow.WindowFocusOut += WindowFocusOut;

            //Can this be done with a property? 
            s_mainWindow.RegisterCustomSchemeHandler("app", AppCustomSchemeUsed);

            s_mainWindow.WaitForClose();

            Console.WriteLine("Done Blocking!");
        }



        //These are the event handlers I'm hooking up
        private static Stream AppCustomSchemeUsed(object? sender, string scheme, string url, out string contentType)
        {
            Log(sender, $"Custom scheme '{scheme}' was used.");
            var currentWindow = (sender as PhotinoWindow)!;

            contentType = "text/javascript";

            var js =
@"
(() =>{
    window.setTimeout(() => {
        const title = document.getElementById('Title');
        const lineage = document.getElementById('Lineage');
        title.innerHTML = "

            + $"'{currentWindow.Title}';" + "\n"

            + $"        lineage.innerHTML = `PhotinoWindow Id: {currentWindow.Id} <br>`;" + "\n";

            //show lineage of this window
            var p = currentWindow.Parent;
            while (p != null)
            {
                js += $"        lineage.innerHTML += `Parent Id: {p.Id} <br>`;" + "\n";
                p = p.Parent;
            }

            js +=
@"        alert(`🎉 Dynamically inserted JavaScript.`);
    }, 1000);
})();
";

            return new MemoryStream(Encoding.UTF8.GetBytes(js));
        }

        private static async void MessageReceivedFromWindow(object? sender, string message)
        {
            Log(sender, $"MessageReceivedFromWindow Callback Fired.");

            var currentWindow = (sender as PhotinoWindow)!;
            if (string.Compare(message, "child-window", true) == 0)
            {
                var iconFile = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                    ? "wwwroot/photino-logo.ico"
                    : "wwwroot/photino-logo.png";

                var x = new PhotinoWindow(currentWindow)
                    .SetTitle($"Child Window {s_windowNumber++}")
                    //.SetIconFile(iconFile)
                    .Load("wwwroot/main.html")

                    //.SetUseOsDefaultLocation(true)
                    //.SetHeight(600)
                    //.SetWidth(800)

                    .SetGrantBrowserPermissions(false)

                    .RegisterWindowCreatingHandler(WindowCreating)
                    .RegisterWindowCreatedHandler(WindowCreated)
                    .RegisterLocationChangedHandler(WindowLocationChanged)
                    .RegisterSizeChangedHandler(WindowSizeChanged)
                    .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                    .RegisterWindowClosingHandler(WindowClosing)

                    .RegisterCustomSchemeHandler("app", AppCustomSchemeUsed)

                    .SetTemporaryFilesPath(currentWindow.TemporaryFilesPath)
                    .SetLogVerbosity(s_logEvents ? 2 : 0);

                x.WaitForClose();

                //x.Center();           //WaitForClose() is non-blocking for child windows
                //x.SetHeight(800);
                //x.Close();
            }
            else if (string.Compare(message, "zoom-in", true) == 0)
            {
                currentWindow.Zoom += 5;
                Log(sender, $"Zoom: {currentWindow.Zoom}");
            }
            else if (string.Compare(message, "zoom-out", true) == 0)
            {
                currentWindow.Zoom -= 5;
                Log(sender, $"Zoom: {currentWindow.Zoom}");
            }
            else if (string.Compare(message, "center", true) == 0)
            {
                currentWindow.Center();
            }
            else if (string.Compare(message, "close", true) == 0)
            {
                currentWindow.Close();
            }
            else if (string.Compare(message, "clearbrowserautofill", true) == 0)
            {
                currentWindow.ClearBrowserAutoFill();
            }
            else if (string.Compare(message, "minimize", true) == 0)
            {
                currentWindow.SetMinimized(!currentWindow.Minimized);
            }
            else if (string.Compare(message, "maximize", true) == 0)
            {
                currentWindow.SetMaximized(!currentWindow.Maximized);
            }
            else if (string.Compare(message, "setcontextmenuenabled", true) == 0)
            {
                currentWindow.SetContextMenuEnabled(!currentWindow.ContextMenuEnabled);
            }
            else if (string.Compare(message, "setdevtoolsenabled", true) == 0)
            {
                currentWindow.SetDevToolsEnabled(!currentWindow.DevToolsEnabled);
            }
            else if (string.Compare(message, "setgrantbrowserpermissions", true) == 0)
            {
                currentWindow.SetGrantBrowserPermissions(!currentWindow.GrantBrowserPermissions);
            }
            else if (string.Compare(message, "seticonfile", true) == 0)
            {
                var iconFile = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                    ? "wwwroot/photino-logo.ico"
                    : "wwwroot/photino-logo.png";

                currentWindow.SetIconFile(iconFile);
            }
            else if (string.Compare(message, "setposition", true) == 0)
            {
                currentWindow.SetLeft(currentWindow.Left + 5);
                currentWindow.SetTop(currentWindow.Top + 5);
            }
            else if (string.Compare(message, "settransparent", true) == 0)
            {
                var t = currentWindow.Transparent;
                Log(sender, $"Transparent: {t}");
                currentWindow.SetTransparent(!t);
            }
            else if (string.Compare(message, "setresizable", true) == 0)
            {
                currentWindow.SetResizable(!currentWindow.Resizable);
            }
            else if (string.Compare(message, "setsize-up", true) == 0)
            {
                currentWindow.SetSize(new Size(currentWindow.Width + 5, currentWindow.Height + 5));
                //currentWindow.SetHeight(currentWindow.Height + 5);
                //currentWindow.SetWidth(currentWindow.Width + 5);
            }
            else if (string.Compare(message, "setsize-down", true) == 0)
            {
                currentWindow.SetSize(new Size(currentWindow.Width - 5, currentWindow.Height - 5));
                //currentWindow.SetHeight(currentWindow.Height - 5);
                //currentWindow.SetWidth(currentWindow.Width - 5);
            }
            else if (string.Compare(message, "settitle", true) == 0)
            {
                currentWindow.SetTitle(currentWindow.Title + "*");
            }
            else if (string.Compare(message, "settopmost", true) == 0)
            {
                currentWindow.SetTopMost(!currentWindow.Topmost);
            }
            else if (string.Compare(message, "setfullscreen", true) == 0)
            {
                currentWindow.SetFullScreen(!currentWindow.FullScreen);
            }
            else if (string.Compare(message, "showproperties", true) == 0)
            {
                var properties = GetPropertiesDisplay(currentWindow);
                currentWindow.ShowMessage("Settings", properties);
            }
            else if (string.Compare(message, "sendWebMessage", true) == 0)
            {
                currentWindow.SendWebMessage("web message 🤖");
            }
            else if (string.Compare(message, "setMinSize", true) == 0)
            {
                currentWindow.SetMinSize(320, 240);
            }
            else if (string.Compare(message, "setMaxSize", true) == 0)
            {
                currentWindow.SetMaxSize(640, 480);
            }
            else if (string.Compare(message, "toastNotification", true) == 0)
            {
                currentWindow.SendNotification("Toast Title", " Toast message! 🤖");
            }
            else if (string.Compare(message, "showOpenFile", true) == 0)
            {
                var results = currentWindow.ShowOpenFile(filters: [
                    ("All files", ["*.*"]),
                    ("Text files", ["*.txt"]),
                    ("Image files", ["*.png", "*.jpg", "*.jpeg"]),
                    ("PDF files", ["*.pdf"]),
                    ("CSharp files", ["*.cs"])
                ]);
                if (results.Length > 0)
                    currentWindow.ShowMessage("Open File", string.Join(Environment.NewLine, results));
                else
                    currentWindow.ShowMessage("Open File", "No file chosen", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showOpenFileAsync", true) == 0)
            {
                var results = await currentWindow.ShowOpenFileAsync(filters: [
                    ("All files", ["*.*"]),
                    ("Text files", ["*.txt"]),
                    ("Image files", ["*.png", "*.jpg", "*.jpeg"]),
                    ("PDF files", ["*.pdf"]),
                    ("CSharp files", ["*.cs"])
                ]);
                if (results.Length > 0)
                    currentWindow.ShowMessage("Open File Async", string.Join(Environment.NewLine, results));
                else
                    currentWindow.ShowMessage("Open File Async", "No file chosen", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showOpenFolder", true) == 0)
            {
                var results = currentWindow.ShowOpenFolder(multiSelect: true);
                if (results.Length > 0)
                    currentWindow.ShowMessage("Open Folder", string.Join(Environment.NewLine, results));
                else
                    currentWindow.ShowMessage("Open Folder", "No folder chosen", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showOpenFolderAsync", true) == 0)
            {
                var results = await currentWindow.ShowOpenFolderAsync(multiSelect: true);
                if (results.Length > 0)
                    currentWindow.ShowMessage("Open Folder Async", string.Join(Environment.NewLine, results));
                else
                    currentWindow.ShowMessage("Open Folder Async", "No folder chosen", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showSaveFile", true) == 0)
            {
                var result = currentWindow.ShowSaveFile("MyTitle", "C:\\", null);
                if (result != null)
                    currentWindow.ShowMessage("Save File", result);
                else
                    currentWindow.ShowMessage("Save File", "File not saved", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showSaveFileAsync", true) == 0)
            {
                var result = await currentWindow.ShowSaveFileAsync("MyTitle", "C:\\", null);
                if (result != null)
                    currentWindow.ShowMessage("Save File Async", result);
                else
                    currentWindow.ShowMessage("Save File Async", "File not saved", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showMessage", true) == 0)
            {
                var result = currentWindow.ShowMessage("Title", "Testing... 🤖");
            }
            else
                throw new Exception($"Unknown message '{message}'");
        }

        private static void WindowCreating(object? sender, EventArgs e)
        {
            Log(sender, "WindowCreating Callback Fired.");
        }

        private static void WindowCreated(object? sender, EventArgs e)
        {
            Log(sender, "WindowCreated Callback Fired.");
        }

        private static void WindowLocationChanged(object? sender, Point location)
        {
            Log(sender, $"WindowLocationChanged Callback Fired.  Left: {location.X}  Top: {location.Y}");
        }

        private static void WindowSizeChanged(object? sender, Size size)
        {
            Log(sender, $"WindowSizeChanged Callback Fired.  Height: {size.Height}  Width: {size.Width}");
        }

        private static void WindowMaximized(object? sender, EventArgs e)
        {
            Log(sender, $"{nameof(WindowMaximized)} Callback Fired.");
        }

        private static void WindowRestored(object? sender, EventArgs e)
        {
            Log(sender, $"{nameof(WindowRestored)} Callback Fired.");
        }

        private static void WindowMinimized(object? sender, EventArgs e)
        {
            Log(sender, $"{nameof(WindowMinimized)} Callback Fired.");
        }

        private static void WindowClosing(object? sender, CancelEventArgs e)
        {
            Log(sender, "WindowClosing Callback Fired.");
        }

        private static void WindowFocusIn(object? sender, EventArgs e)
        {
            Log(sender, "WindowFocusIn Callback Fired.");
        }

        private static void WindowFocusOut(object? sender, EventArgs e)
        {
            Log(sender, "WindowFocusOut Callback Fired.");
        }




        private static string GetPropertiesDisplay(PhotinoWindow currentWindow)
        {
            var sb = new StringBuilder();
            sb.AppendLine($"Title: {currentWindow.Title}");
            sb.AppendLine($"Zoom: {currentWindow.Zoom}");
            sb.AppendLine();
            sb.AppendLine($"ContextMenuEnabled: {currentWindow.ContextMenuEnabled}");
            sb.AppendLine($"DevToolsEnabled: {currentWindow.DevToolsEnabled}");
            sb.AppendLine($"GrantBrowserPermissions: {currentWindow.GrantBrowserPermissions}");
            sb.AppendLine();
            sb.AppendLine($"Top: {currentWindow.Top}");
            sb.AppendLine($"Left: {currentWindow.Left}");
            sb.AppendLine($"Height: {currentWindow.Height}");
            sb.AppendLine($"Width: {currentWindow.Width}");
            sb.AppendLine();
            sb.AppendLine($"Resizable: {currentWindow.Resizable}");
            sb.AppendLine($"Screen DPI: {currentWindow.ScreenDpi}");
            sb.AppendLine($"Topmost: {currentWindow.Topmost}");
            sb.AppendLine($"Maximized: {currentWindow.Maximized}");
            sb.AppendLine($"Minimized: {currentWindow.Minimized}");

            return sb.ToString();
        }

        private static void Log(object? sender, string message)
        {
            if (!s_logEvents) return;
            var windowTitle = sender is PhotinoWindow currentWindow ? currentWindow.Title : string.Empty;
            Console.WriteLine($"-Client App: \"{windowTitle}\" {message}");
        }
    }
}
