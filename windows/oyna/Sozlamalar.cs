// «Özbek klaviaturasi» sozlamalari — Windows (.NET Framework 4.8, WinForms; Linux'da Mono bilan yig'iladi).
// Har funksiya alohida toggle; sozlama HKCU\Software\YangiKlaviatura ga yoziladi va TSF xizmati uni darhol ko'radi.
//   Sozlamalar.exe                 — oyna
//   Sozlamalar.exe --skrin a.png   — oynani rasmga oladi va yopiladi (sinov uchun)
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Win32;

[assembly: System.Reflection.AssemblyTitle("Özbek klaviaturasi")]
[assembly: System.Reflection.AssemblyProduct("Özbek klaviaturasi")]
[assembly: System.Reflection.AssemblyCompany("adkhamjon1")]
[assembly: System.Reflection.AssemblyCopyright("© 2026 adkhamjon1")]

namespace YangiKlaviatura
{
    static class Mavzu
    {
        public static bool Qorongi;
        public static Color Fon, Karta, Chiziq, Matn, Xira, Urgu, UrguMatn, Bildirish;
        public static string Shrift = "Segoe UI", Ikonka;
        public static float K = 1f;  // ekran masshtabi (96 DPI = 1)
        public static int D(float px) { return (int)Math.Round(px * K); }

        public static void Aniqla()
        {
            object yorug = Registry.GetValue(@"HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize",
                                             "AppsUseLightTheme", 1);
            Qorongi = yorug is int && (int)yorug == 0;
            Urgu = Color.FromArgb(42, 98, 201);
            object rang = Registry.GetValue(@"HKEY_CURRENT_USER\Software\Microsoft\Windows\DWM", "AccentColor", null);
            if (rang is int)
            {
                int abgr = (int)rang;
                Urgu = Color.FromArgb(abgr & 0xFF, (abgr >> 8) & 0xFF, (abgr >> 16) & 0xFF);
            }
            Fon = Qorongi ? Color.FromArgb(32, 32, 32) : Color.FromArgb(243, 243, 243);
            Karta = Qorongi ? Color.FromArgb(43, 43, 43) : Color.White;
            Chiziq = Qorongi ? Color.FromArgb(58, 58, 58) : Color.FromArgb(229, 229, 229);
            Matn = Qorongi ? Color.FromArgb(240, 240, 240) : Color.FromArgb(27, 27, 27);
            Xira = Qorongi ? Color.FromArgb(160, 160, 160) : Color.FromArgb(96, 96, 96);
            UrguMatn = Color.White;
            Bildirish = Qorongi ? Color.FromArgb(60, 60, 60) : Color.FromArgb(32, 32, 32);
            var bor = new HashSet<string>(FontFamily.Families.Select(f => f.Name));
            if (bor.Contains("Segoe UI Variable Text")) Shrift = "Segoe UI Variable Text";
            Ikonka = bor.Contains("Segoe Fluent Icons") ? "Segoe Fluent Icons" : bor.Contains("Segoe MDL2 Assets") ? "Segoe MDL2 Assets" : null;
        }

        public static Font F(float olcham, FontStyle uslub = FontStyle.Regular) { return new Font(Shrift, olcham, uslub); }

        public static GraphicsPath Dumaloq(RectangleF r, float radius)
        {
            var yol = new GraphicsPath();
            float d = radius * 2;
            yol.AddArc(r.X, r.Y, d, d, 180, 90);
            yol.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            yol.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            yol.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            yol.CloseFigure();
            return yol;
        }
    }

    // Toggle: dumaloq tugmali yoqgich (Windows 11 uslubida)
    class Yoqgich : Control
    {
        bool holat;
        public event EventHandler Ozgardi;

        public Yoqgich()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.UserPaint |
                     ControlStyles.ResizeRedraw | ControlStyles.SupportsTransparentBackColor | ControlStyles.Selectable, true);
            Size = new Size(Mavzu.D(44), Mavzu.D(22));
            Cursor = Cursors.Hand;
            BackColor = Color.Transparent;
            TabStop = true;
        }

        public bool Holat
        {
            get { return holat; }
            set { if (holat != value) { holat = value; Invalidate(); } }
        }

        void Bos()
        {
            if (!Enabled) return;
            Holat = !Holat;
            if (Ozgardi != null) Ozgardi(this, EventArgs.Empty);
        }

        protected override void OnClick(EventArgs e) { base.OnClick(e); Focus(); Bos(); }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);
            if (e.KeyCode == Keys.Space || e.KeyCode == Keys.Enter) { Bos(); e.Handled = true; }
        }

        protected override void OnEnabledChanged(EventArgs e) { base.OnEnabledChanged(e); Invalidate(); }
        protected override void OnGotFocus(EventArgs e) { base.OnGotFocus(e); Invalidate(); }
        protected override void OnLostFocus(EventArgs e) { base.OnLostFocus(e); Invalidate(); }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            var r = new RectangleF(1, 1, Width - 3, Height - 3);
            float radius = r.Height / 2;
            int alfa = Enabled ? 255 : 90;
            using (var yol = Mavzu.Dumaloq(r, radius))
            {
                if (holat)
                    using (var b = new SolidBrush(Color.FromArgb(alfa, Mavzu.Urgu))) g.FillPath(b, yol);
                else
                    using (var p = new Pen(Color.FromArgb(alfa, Mavzu.Xira), 1.2f)) g.DrawPath(p, yol);
            }
            float d = holat ? r.Height - 8 : r.Height - 10;
            float x = holat ? r.Right - d - 4 : r.X + 5;
            using (var b = new SolidBrush(Color.FromArgb(alfa, holat ? Mavzu.UrguMatn : Mavzu.Xira)))
                g.FillEllipse(b, x, r.Y + (r.Height - d) / 2, d, d);
            if (Focused)
                using (var p = new Pen(Mavzu.Matn, 1f) { DashStyle = DashStyle.Dot }) g.DrawRectangle(p, 0, 0, Width - 1, Height - 1);
        }
    }

    // Oq (qorong'i) burchagi dumaloq karta; ichida qatorlar ustma-ust turadi
    class Karta : Panel
    {
        public Karta()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.UserPaint |
                     ControlStyles.ResizeRedraw, true);
            BackColor = Mavzu.Fon;
            Padding = new Padding(1);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            using (var yol = Mavzu.Dumaloq(new RectangleF(0.5f, 0.5f, Width - 1.5f, Height - 1.5f), Mavzu.D(8)))
            using (var b = new SolidBrush(Mavzu.Karta))
            using (var p = new Pen(Mavzu.Chiziq))
            {
                g.FillPath(b, yol);
                g.DrawPath(p, yol);
            }
            // qatorlar orasidagi chiziq
            using (var p = new Pen(Mavzu.Chiziq))
                for (int i = 0; i < Controls.Count - 1; i++)
                    g.DrawLine(p, Mavzu.D(16), Controls[i].Bottom, Width - Mavzu.D(16), Controls[i].Bottom);
        }

        public void Joyla()
        {
            int y = 1;
            foreach (Control c in Controls)
            {
                c.SetBounds(1, y, Width - 2, c.Height);
                y += c.Height;
            }
            Height = y + 1;
        }

        protected override void OnResize(EventArgs e) { base.OnResize(e); Joyla(); }
    }

    // Karta ichidagi qator: ikonka, nom, izoh va o'ngda boshqaruv elementi
    class Qator : Panel
    {
        readonly Label nom, izoh, belgi;
        public readonly Control Ong;

        public Qator(string ikonka, string sarlavha, string tavsif, Control ong)
        {
            BackColor = Mavzu.Karta;
            belgi = new Label { Text = ikonka ?? "", Font = Mavzu.Ikonka != null ? new Font(Mavzu.Ikonka, 13f) : Mavzu.F(10f),
                                ForeColor = Mavzu.Matn, AutoSize = false, TextAlign = ContentAlignment.MiddleCenter,
                                Visible = Mavzu.Ikonka != null && !string.IsNullOrEmpty(ikonka) };
            nom = new Label { Text = sarlavha, Font = Mavzu.F(10.5f), ForeColor = Mavzu.Matn, AutoSize = false, AutoEllipsis = true };
            izoh = new Label { Text = tavsif ?? "", Font = Mavzu.F(9f), ForeColor = Mavzu.Xira, AutoSize = false, AutoEllipsis = true };
            Controls.Add(belgi);
            Controls.Add(nom);
            Controls.Add(izoh);
            Ong = ong;
            if (ong != null) Controls.Add(ong);
            bool ikki = !string.IsNullOrEmpty(tavsif);  // balandlik shriftdan: masshtab va shrift almashsa ham sig'adi
            Height = Math.Max(Mavzu.D(ikki ? 64 : 50), (ikki ? nom.Font.Height + izoh.Font.Height + Mavzu.D(2) : nom.Font.Height) + Mavzu.D(26));
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            if (belgi == null) return;
            int chap = belgi.Visible ? Mavzu.D(56) : Mavzu.D(18);
            belgi.SetBounds(Mavzu.D(14), 0, Mavzu.D(32), Height);
            int ongEn = Ong != null ? Ong.Width + Mavzu.D(24) : Mavzu.D(16);
            bool ikki = izoh.Text.Length > 0;
            int h1 = nom.Font.Height, h2 = ikki ? izoh.Font.Height : 0;
            int y = (Height - h1 - (ikki ? h2 + Mavzu.D(2) : 0)) / 2;
            nom.TextAlign = ContentAlignment.MiddleLeft;
            nom.SetBounds(chap, y, Width - chap - ongEn, h1);
            izoh.SetBounds(chap, y + h1 + Mavzu.D(2), Width - chap - ongEn, h2);
            izoh.Visible = ikki;
            if (Ong != null) Ong.Location = new Point(Width - Ong.Width - Mavzu.D(18), (Height - Ong.Height) / 2);
        }

        public void Faol(bool faol)
        {
            nom.ForeColor = faol ? Mavzu.Matn : Mavzu.Xira;
            if (Ong != null) Ong.Enabled = faol;
        }
    }

    // Sozlamalar: HKCU\Software\YangiKlaviatura (DWORD 0/1) — TSF xizmati shu yerdan o'qiydi
    static class Reestr
    {
        const string Yol = @"Software\YangiKlaviatura";
        public static readonly Dictionary<string, bool> Standart = new Dictionary<string, bool> {
            {"tugmalar", true}, {"lotin", true}, {"sh_ajrat", true}, {"eski_belgi", false}, {"kirill", true},
            {"darhol", true}, {"istisno", true}, {"lugat", true}, {"organish", true}, {"tutuq", false},
        };

        public static bool Oqi(string kalit)
        {
            using (var k = Registry.CurrentUser.OpenSubKey(Yol))
            {
                object v = k != null ? k.GetValue(kalit) : null;
                return v is int ? (int)v != 0 : Standart[kalit];
            }
        }

        public static void Yoz(string kalit, bool qiymat)
        {
            using (var k = Registry.CurrentUser.CreateSubKey(Yol))
                k.SetValue(kalit, qiymat ? 1 : 0, RegistryValueKind.DWord);
        }
    }

    // Til ro'yxatidagi rejimlar (TSF profillari). Holat reestrdan o'qiladi, o'zgartirish input.dll
    // InstallLayoutOrTip bilan — o'rnatuvchi ham shuni chaqiradi (yashirin PowerShell antivirusga shubhali ko'rinadi).
    static class Tillar
    {
        public const string Clsid = "{1B5150A6-39E7-42B1-B771-BBF2A214010B}";
        public const string Oz = "{1090CF6A-22FC-486D-A2C4-566BFB57567F}";
        public const string Eski = "{45A97DD2-5AD2-40C8-9E36-A64C2860C9F5}";
        public const string Kirill = "{064D6F63-6C7F-4EA4-AF76-E4C8FF0B2165}";

        static string Tip(string profil) { return "0443:" + Clsid + profil; }

        public static bool Bormi(string profil)
        {
            using (var k = Registry.CurrentUser.OpenSubKey(@"Control Panel\International\User Profile"))
            {
                if (k == null) return false;
                foreach (string til in k.GetSubKeyNames())
                {
                    if (!til.StartsWith("uz-Latn", StringComparison.OrdinalIgnoreCase)) continue;
                    using (var t = k.OpenSubKey(til))
                        if (t != null && t.GetValueNames().Any(n => string.Equals(n, Tip(profil), StringComparison.OrdinalIgnoreCase)))
                            return true;
                }
            }
            return false;
        }

        [DllImport("input.dll", CharSet = CharSet.Unicode)]
        static extern bool InstallLayoutOrTip(string tiplar, uint bayroq);
        const uint OLIB_TASHLA = 1;  // ILOT_UNINSTALL

        // Rejimlarni joriy foydalanuvchining til ro'yxatiga qo'shadi yoki oladi; o'zbek tili bo'lmasa Windows o'zi qo'shadi
        public static Task<bool> Ozgartir(IEnumerable<string> qosh, IEnumerable<string> ol)
        {
            string q = string.Join(";", qosh.Select(Tip)), o = string.Join(";", ol.Select(Tip));
            return Task.Run(() =>
            {
                try
                {
                    bool bajarildi = true;
                    if (q.Length > 0) bajarildi &= InstallLayoutOrTip(q, 0);
                    if (o.Length > 0) bajarildi &= InstallLayoutOrTip(o, OLIB_TASHLA);
                    return bajarildi;
                }
                catch { return false; }
            });
        }
    }

    // Foydalanuvchi lug'ati: %APPDATA%\YangiKlaviatura\lugat.txt
    static class Lugat
    {
        public static string Fayl
        {
            get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "YangiKlaviatura", "lugat.txt"); }
        }

        public static List<string> Sozlar()
        {
            try
            {
                return File.ReadAllLines(Fayl, Encoding.UTF8).Select(q => q.Split('#')[0].Trim())
                           .Where(q => q.Length > 0).Distinct(StringComparer.OrdinalIgnoreCase)
                           .OrderBy(q => q, StringComparer.OrdinalIgnoreCase).ToList();
            }
            catch { return new List<string>(); }
        }

        public static void Yoz(IEnumerable<string> sozlar)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Fayl));
            File.WriteAllLines(Fayl, sozlar, new UTF8Encoding(false));
        }
    }

    // O'girish — TSF xizmatining C++ yadrosi (ozbek64.dll / ozbek32.dll)
    static class Ogirgich
    {
        [DllImport("ozbek64.dll", EntryPoint = "ozbek_ogir", CharSet = CharSet.Unicode)]
        static extern int Ogir64(string matn, StringBuilder natija, int sigim, uint bayroqlar);
        [DllImport("ozbek32.dll", EntryPoint = "ozbek_ogir", CharSet = CharSet.Unicode)]
        static extern int Ogir32(string matn, StringBuilder natija, int sigim, uint bayroqlar);

        public static string Ogir(string matn, uint bayroqlar)
        {
            try
            {
                var sb = new StringBuilder(matn.Length * 2 + 16);
                int n = Environment.Is64BitProcess ? Ogir64(matn, sb, sb.Capacity, bayroqlar) : Ogir32(matn, sb, sb.Capacity, bayroqlar);
                return n >= 0 ? sb.ToString() : matn;
            }
            catch (Exception) { return null; }  // DLL yo'q (masalan, Linux'da oldindan ko'rish)
        }
    }

    class Oyna : Form
    {
        const string Versiya = "1.0.0";
        readonly Panel varaq = new Panel { AutoScroll = true, Dock = DockStyle.Fill };
        readonly List<Control> ustun = new List<Control>();
        readonly Dictionary<string, Yoqgich> yoqgichlar = new Dictionary<string, Yoqgich>();
        readonly Dictionary<string, Qator> qatorlar = new Dictionary<string, Qator>();
        readonly Label bildirish = new Label();
        readonly Timer bildirishVaqti = new Timer { Interval = 3500 };
        Panel banner;
        Karta lugatKarta;
        TextBox kirish, natija;
        Panel tugmalarRasmi;

        public Oyna()
        {
            Text = "Özbek klaviaturasi";
            Font = Mavzu.F(9.5f);
            BackColor = Mavzu.Fon;
            ForeColor = Mavzu.Matn;
            ClientSize = new Size(Mavzu.D(720), Mavzu.D(860));
            MinimumSize = new Size(Mavzu.D(520), Mavzu.D(480));
            StartPosition = FormStartPosition.CenterScreen;
            try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }
            DoubleBuffered = true;
            Controls.Add(varaq);
            varaq.Resize += (s, e) => Joyla();

            Qur();
            bildirish.Visible = false;
            bildirish.AutoSize = false;
            bildirish.TextAlign = ContentAlignment.MiddleCenter;
            bildirish.Font = Mavzu.F(9.5f);
            bildirish.BackColor = Mavzu.Bildirish;
            bildirish.ForeColor = Color.White;
            Controls.Add(bildirish);
            bildirish.BringToFront();
            bildirishVaqti.Tick += (s, e) => { bildirish.Visible = false; bildirishVaqti.Stop(); };
            Activated += (s, e) => { ReestrdanYangila(); LugatniYangila(); BannerniYangila(); };
        }

        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            try
            {
                int qora = Mavzu.Qorongi ? 1 : 0;
                DwmSetWindowAttribute(Handle, 20, ref qora, 4);  // qorong'i sarlavha (Windows 10 2004+)
            }
            catch { }
        }

        [DllImport("dwmapi.dll")]
        static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int qiymat, int hajm);

        // -- qurish --

        void Qur()
        {
            banner = BannerYasa();
            Qosh(banner);
            Qosh(Hero());
            Qosh(Izohli("", "Bir vaqtda bitta rejim ishlaydi",
                "Rejim Win+Probel bilan tanlanadi. Har bo'limdagi sozlama faqat o'sha rejimga tegishli."));

            Bolim("Yangi lotin", "ÖZ", "Lotincha yoziladi, yangi harflar o'zi chiqadi.", new[] {
                Toggle("tugmalar", "", "Yangi harflar tugmalarda", "W → ş, C → ç, [ → ö, ] → ğ. Asl belgi o'ng Alt bilan yoziladi"),
                Toggle("lotin", "", "Eski yozuvdan o'girish", "sh → ş, ch → ç, o' → ö, g' → ğ"),
                Toggle("sh_ajrat", "", "s'h → sh", "Is'hoq → Ishoq: s va h alohida o'qiladi"),
            });
            Bolim("Eski lotin", "O'Z", "Hozirgi alifbo o'zgarishsiz yoziladi.", new[] {
                RoyxatToggle("eski_royxat", Tillar.Eski, "Ro'yxatda ko'rinadi", "O'chirilsa «o'z» til ro'yxatidan olinadi"),
                Toggle("eski_belgi", "", "oʻ va gʻ belgisi", "o' → oʻ, g' → gʻ (U+02BB)"),
            });
            Bolim("Kirill", "ЎЗ", "O'zbek kirill klaviaturasida yoziladi.", new[] {
                RoyxatToggle("kirill_royxat", Tillar.Kirill, "Ro'yxatda ko'rinadi", "O'chirilsa «ўз» til ro'yxatidan olinadi"),
                Toggle("kirill", "", "Yangi lotinga o'girish", "шаҳар → şahar, ўғил → öğil. O'chirilsa oddiy kirillcha yoziladi"),
            });
            Bolim("Aqlli o'girish", null, "So'z tugaganda tekshiriladi: xorijiy so'z va havola asliga qaytadi.", new[] {
                Toggle("darhol", "", "Darhol yozish", "Harf darhol yoziladi, o'girish joyida bo'ladi"),
                Toggle("istisno", "", "Havola va email'ga tegmaslik", "www.shifo.uz, info@choy.uz, ch1, Schwann o'zgarmaydi"),
                Toggle("lugat", "", "Xorijiy so'zlar lug'ati", "Chrome, Shell, CHARGE va yana 180 dan ortiq so'z o'girilmaydi"),
                Toggle("organish", "", "Qaytarilgan so'zni eslab qolish", "So'zdan keyin darhol Backspace bosilsa, so'z asliga qaytadi va lug'atga yoziladi"),
            });
            lugatKarta = new Karta();
            Qosh(Sarlavha("Mening lug'atim", null, "Shu so'zlar o'girilmaydi. Backspace bilan qaytarilgan so'z ham shu yerga tushadi."));
            Qosh(lugatKarta);
            Bolim("Umumiy", null, "Parol maydonida o'girish doim o'chiq bo'ladi.", new[] {
                Toggle("tutuq", "", "Tutuq belgisi ʼ", "ma'no → maʼno, маъно → maʼno (U+02BC)"),
            });
            Qosh(Sarlavha("Sinab ko'rish", null, "Eski lotin yoki kirill yozuvidagi matn shu yerda yangi alifboga o'giriladi."));
            Qosh(Sinov());
            Bolim("Tezkor tugmalar", null, null, new[] {
                new Qator("", "Rejimni almashtirish", "Til panelida tanlash ham mumkin", Yorliq("Win + Probel")),
                new Qator("", "Til sozlamalari", "Windows sozlamalaridagi til va klaviaturalar", Tugma("Ochish", () => Och("ms-settings:regionlanguage"))),
            });
            Qosh(Pastki());
            Yangila();
            LugatniYangila();
            BannerniYangila();
        }

        void Qosh(Control c) { ustun.Add(c); varaq.Controls.Add(c); }

        void Joyla()
        {
            int en = Math.Min(varaq.ClientSize.Width - Mavzu.D(48), Mavzu.D(820));
            int x = (varaq.ClientSize.Width - en) / 2, y = Mavzu.D(20) + varaq.AutoScrollPosition.Y;
            foreach (var c in ustun)
            {
                if (!c.Visible) continue;
                c.SetBounds(x, y, en, c.Height);
                if (c is Karta) ((Karta)c).Joyla();
                y += c.Height + Mavzu.D(c.Tag as string == "sarlavha" ? 6 : 14);
            }
            if (bildirish.Visible) BildirishniJoyla();
        }

        Control Sarlavha(string nom, string belgi, string izoh)
        {
            var n = new Label { Text = nom, Font = Mavzu.F(11f, FontStyle.Bold), ForeColor = Mavzu.Matn, AutoSize = true, Location = new Point(Mavzu.D(2), Mavzu.D(2)) };
            var izohShrift = Mavzu.F(9f);
            var p = new Panel { Tag = "sarlavha", BackColor = Mavzu.Fon,
                                Height = n.Font.Height + Mavzu.D(8) + (izoh != null ? izohShrift.Height + Mavzu.D(4) : 0) };
            p.Controls.Add(n);
            if (belgi != null)
            {
                var b = new Label { Text = belgi, Font = Mavzu.F(8.5f, FontStyle.Bold), ForeColor = Mavzu.UrguMatn, BackColor = Mavzu.Urgu,
                                    AutoSize = true, Padding = new Padding(6, 2, 6, 2) };
                p.Controls.Add(b);
                p.Resize += (s, e) => b.Location = new Point(p.Width - b.Width - Mavzu.D(2), Mavzu.D(4));
            }
            if (izoh != null)
            {
                var iz = new Label { Text = izoh, Font = izohShrift, ForeColor = Mavzu.Xira, AutoSize = false, AutoEllipsis = true };
                p.Controls.Add(iz);
                p.Resize += (s, e) => iz.SetBounds(Mavzu.D(2), n.Font.Height + Mavzu.D(6), p.Width - Mavzu.D(4), izohShrift.Height + Mavzu.D(2));
            }
            return p;
        }

        void Bolim(string nom, string belgi, string izoh, Qator[] qatorlarRoyxati)
        {
            Qosh(Sarlavha(nom, belgi, izoh));
            var k = new Karta();
            foreach (var q in qatorlarRoyxati) k.Controls.Add(q);
            Qosh(k);
        }

        Qator Toggle(string kalit, string ikonka, string nom, string izoh)
        {
            var y = new Yoqgich { Holat = Reestr.Oqi(kalit) };
            y.Ozgardi += (s, e) =>
            {
                Reestr.Yoz(kalit, y.Holat);
                Xabar(XabarMatni(kalit, y.Holat));
                Yangila();
            };
            yoqgichlar[kalit] = y;
            var q = new Qator(ikonka, nom, izoh, y);
            qatorlar[kalit] = q;
            return q;
        }

        Qator RoyxatToggle(string kalit, string profil, string nom, string izoh)
        {
            var y = new Yoqgich { Holat = Tillar.Bormi(profil) };
            y.Ozgardi += async (s, e) =>
            {
                bool yoq = y.Holat;
                y.Enabled = false;
                bool ok = await Tillar.Ozgartir(yoq ? new[] { profil } : new string[0], yoq ? new string[0] : new[] { profil });
                y.Enabled = true;
                y.Holat = Tillar.Bormi(profil);
                string nomi = profil == Tillar.Eski ? "«o'z»" : "«ўз»";
                Xabar(!ok ? "Til ro'yxatini o'zgartirib bo'lmadi." : yoq ? nomi + " qo'shildi: Win+Probel bilan tanlanadi." : nomi + " til ro'yxatidan olindi.");
                Yangila();
            };
            yoqgichlar[kalit] = y;
            var q = new Qator("", nom, izoh, y);
            qatorlar[kalit] = q;
            return q;
        }

        static string XabarMatni(string kalit, bool yoq)
        {
            switch (kalit)
            {
                case "tugmalar": return yoq ? "W, C, [, ] tugmalarida yangi harflar chiqadi." : "W, C, [, ] tugmalari asl belgisini yozadi.";
                case "lotin": return yoq ? "«öz» da sh, ch, o', g' yangi harfga aylanadi." : "«öz» da sh, ch, o', g' o'zgarmaydi.";
                case "sh_ajrat": return yoq ? "s'h ajratiladi: Is'hoq → Ishoq." : "s'h o'zgarishsiz qoladi.";
                case "eski_belgi": return yoq ? "«o'z» da o' → oʻ, g' → gʻ yoziladi." : "«o'z» da apostrof odatdagi ' bo'ladi.";
                case "kirill": return yoq ? "«ўз» da matn yangi lotinda chiqadi." : "«ўз» da oddiy kirillcha yoziladi.";
                case "darhol": return yoq ? "Harf darhol yoziladi, o'girish joyida bo'ladi." : "Qaror kutayotgan harf tagiga chizilib turadi.";
                case "istisno": return yoq ? "Havola, email va xorijiy so'z o'girilmaydi." : "Havola va email ham o'giriladi.";
                case "lugat": return yoq ? "Xorijiy so'zlar lug'ati ishlaydi." : "Lug'at o'chdi: «Chrome» ham «Çrome» bo'ladi.";
                case "organish": return yoq ? "Qaytarilgan so'z lug'atga yoziladi." : "Qaytarilgan so'z lug'atga yozilmaydi.";
                case "tutuq": return yoq ? "So'z ichidagi apostrof ʼ bo'ladi." : "Apostrof odatdagi ' bo'lib qoladi.";
            }
            return "";
        }

        Control Hero()
        {
            var p = new Panel { BackColor = Mavzu.Fon };
            tugmalarRasmi = new Panel { Size = new Size(Mavzu.D(4 * 76 + 3 * 12), Mavzu.D(84)), BackColor = Mavzu.Fon };
            tugmalarRasmi.Paint += (s, e) =>
            {
                var g = e.Graphics;
                g.SmoothingMode = SmoothingMode.AntiAlias;
                g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.AntiAlias;
                string[,] t = { { "ö", "[" }, { "ğ", "]" }, { "ş", "W" }, { "ç", "C" } };
                int alfa = Reestr.Oqi("tugmalar") ? 255 : 90;
                for (int i = 0; i < 4; i++)
                {
                    var r = new RectangleF(Mavzu.D(i * 88) + 1, 1, Mavzu.D(74), Mavzu.D(78));
                    using (var yol = Mavzu.Dumaloq(r, Mavzu.D(12)))
                    using (var b = new SolidBrush(Color.FromArgb(alfa, Mavzu.Karta)))
                    using (var c = new Pen(Color.FromArgb(alfa, Mavzu.Chiziq)))
                    {
                        g.FillPath(b, yol);
                        g.DrawPath(c, yol);
                    }
                    var markaz = new StringFormat { Alignment = StringAlignment.Center, LineAlignment = StringAlignment.Center };
                    using (var f = Mavzu.F(24f, FontStyle.Bold))
                    using (var b = new SolidBrush(Color.FromArgb(alfa, Mavzu.Urgu)))
                        g.DrawString(t[i, 0], f, b, new RectangleF(r.X, r.Y + Mavzu.D(2), r.Width, Mavzu.D(52)), markaz);
                    using (var f = new Font("Consolas", 9f, FontStyle.Bold))
                    using (var b = new SolidBrush(Color.FromArgb(alfa, Mavzu.Xira)))
                        g.DrawString(t[i, 1], f, b, new RectangleF(r.X, r.Y + Mavzu.D(50), r.Width, Mavzu.D(22)), markaz);
                }
            };
            var sarlavha = new Label { Text = "Yangi o'zbek yozuvi", Font = Mavzu.F(18f, FontStyle.Bold), ForeColor = Mavzu.Matn,
                                       AutoSize = false, TextAlign = ContentAlignment.MiddleCenter };
            var izoh = new Label { Text = "Ö, Ğ, Ş va Ç harflari 22.04.2027 dan rasmiy alifboga kiradi.", Font = Mavzu.F(10f),
                                   ForeColor = Mavzu.Xira, AutoSize = false, TextAlign = ContentAlignment.MiddleCenter };
            p.Controls.Add(tugmalarRasmi);
            p.Controls.Add(sarlavha);
            p.Controls.Add(izoh);
            int y1 = Mavzu.D(100), h1 = sarlavha.Font.Height + Mavzu.D(6), h2 = izoh.Font.Height + Mavzu.D(4);
            p.Height = y1 + h1 + h2 + Mavzu.D(10);
            p.Resize += (s, e) =>
            {
                tugmalarRasmi.Location = new Point((p.Width - tugmalarRasmi.Width) / 2, Mavzu.D(6));
                sarlavha.SetBounds(0, y1, p.Width, h1);
                izoh.SetBounds(0, y1 + h1, p.Width, h2);
            };
            return p;
        }

        Control Izohli(string ikonka, string nom, string izoh)
        {
            var k = new Karta();
            k.Controls.Add(new Qator(ikonka, nom, izoh, null));
            return k;
        }

        Panel BannerYasa()
        {
            var p = new Panel { Height = Mavzu.D(56), BackColor = Mavzu.Urgu, Visible = false };
            var l = new Label { Text = "Rejimlar hali yoqilmagan: «ÖZ», «O'Z» va «ЎЗ» til ro'yxatiga qo'shiladi.",
                                ForeColor = Mavzu.UrguMatn, Font = Mavzu.F(10f), AutoSize = false, TextAlign = ContentAlignment.MiddleLeft };
            var b = new Button { Text = "Yoqish", FlatStyle = FlatStyle.Flat, BackColor = Mavzu.UrguMatn, ForeColor = Mavzu.Urgu,
                                 Font = Mavzu.F(9.5f, FontStyle.Bold), Size = new Size(Mavzu.D(120), Mavzu.D(32)), Cursor = Cursors.Hand };
            b.FlatAppearance.BorderSize = 0;
            b.Click += async (s, e) =>
            {
                b.Enabled = false;
                b.Text = "Yoqilmoqda…";
                bool ok = await Tillar.Ozgartir(new[] { Tillar.Oz, Tillar.Eski, Tillar.Kirill }, new string[0]);
                b.Text = "Yoqish";
                b.Enabled = true;
                Xabar(ok ? "Rejimlar yoqildi: Win+Probel bilan tanlanadi." : "Til ro'yxatini o'zgartirib bo'lmadi.");
                BannerniYangila();
                foreach (var k in new[] { "eski_royxat", "kirill_royxat" })
                    yoqgichlar[k].Holat = Tillar.Bormi(k == "eski_royxat" ? Tillar.Eski : Tillar.Kirill);
            };
            p.Controls.Add(l);
            p.Controls.Add(b);
            p.Resize += (s, e) =>
            {
                b.Location = new Point(p.Width - b.Width - Mavzu.D(14), (p.Height - b.Height) / 2);
                l.SetBounds(Mavzu.D(16), 0, p.Width - b.Width - Mavzu.D(40), p.Height);
            };
            return p;
        }

        Control Sinov()
        {
            var k = new Karta();
            var quti = new Panel { BackColor = Mavzu.Karta };
            kirish = new TextBox { Multiline = true, BorderStyle = BorderStyle.None, BackColor = Mavzu.Karta, ForeColor = Mavzu.Matn,
                                   Font = Mavzu.F(11f), Text = "O'zbekiston — cho'l va tog'lar yurti. Тошкент — қадимий шаҳар. Chrome: www.shifo.uz" };
            natija = new TextBox { Multiline = true, ReadOnly = true, BorderStyle = BorderStyle.None, BackColor = Mavzu.Karta, ForeColor = Mavzu.Urgu,
                                   Font = Mavzu.F(11f, FontStyle.Bold) };
            var nusxa = Tugma("Nusxa olish", () => { if (natija.Text.Length > 0) { Clipboard.SetText(natija.Text); Xabar("Nusxa olindi."); } });
            kirish.TextChanged += (s, e) => SinovniYangila();
            quti.Controls.Add(kirish);
            quti.Controls.Add(natija);
            quti.Controls.Add(nusxa);
            int h = kirish.Font.Height * 2 + Mavzu.D(6);
            quti.Height = Mavzu.D(14) + h + Mavzu.D(12) + h + Mavzu.D(14);
            quti.Resize += (s, e) =>
            {
                kirish.SetBounds(Mavzu.D(18), Mavzu.D(14), quti.Width - Mavzu.D(36), h);
                natija.SetBounds(Mavzu.D(18), Mavzu.D(26) + h, quti.Width - Mavzu.D(48) - nusxa.Width, h);
                nusxa.Location = new Point(quti.Width - nusxa.Width - Mavzu.D(18), Mavzu.D(26) + h + (h - nusxa.Height) / 2);
            };
            k.Controls.Add(quti);
            return k;
        }

        Control Pastki()
        {
            var l = new LinkLabel { Text = "Özbek klaviaturasi " + Versiya + " · github.com/adkhamjon1/yangi-klaviatura", AutoSize = false,
                                    TextAlign = ContentAlignment.MiddleCenter, Height = Mavzu.D(40), Font = Mavzu.F(9f), LinkColor = Mavzu.Xira,
                                    ActiveLinkColor = Mavzu.Urgu, BackColor = Mavzu.Fon, LinkBehavior = LinkBehavior.HoverUnderline };
            l.LinkClicked += (s, e) => Och("https://github.com/adkhamjon1/yangi-klaviatura");
            return l;
        }

        Button Tugma(string matn, Action bosildi)
        {
            var b = new Button { Text = matn, FlatStyle = FlatStyle.Flat, BackColor = Mavzu.Fon, ForeColor = Mavzu.Matn, Font = Mavzu.F(9.5f),
                                 AutoSize = true, MinimumSize = new Size(0, Mavzu.D(32)), Cursor = Cursors.Hand, Padding = new Padding(Mavzu.D(10), 0, Mavzu.D(10), 0) };
            b.FlatAppearance.BorderColor = Mavzu.Chiziq;
            b.Click += (s, e) => bosildi();
            return b;
        }

        static Label Yorliq(string matn)
        {
            return new Label { Text = matn, AutoSize = true, Font = Mavzu.F(9.5f, FontStyle.Bold), ForeColor = Mavzu.Matn,
                               BackColor = Mavzu.Fon, Padding = new Padding(8, 4, 8, 4) };
        }

        // -- holat --

        // Toggle til panelidagi menyudan ham o'zgaradi: oyna qaytib ochilganda reestrdan qayta o'qiladi
        void ReestrdanYangila()
        {
            bool ozgardi = false;
            foreach (var j in yoqgichlar)
            {
                if (!Reestr.Standart.ContainsKey(j.Key)) continue;
                bool v = Reestr.Oqi(j.Key);
                if (j.Value.Holat != v) { j.Value.Holat = v; ozgardi = true; }
            }
            if (ozgardi) Yangila();
        }

        void Yangila()
        {
            if (qatorlar.ContainsKey("eski_belgi")) qatorlar["eski_belgi"].Faol(yoqgichlar["eski_royxat"].Holat);
            if (qatorlar.ContainsKey("kirill")) qatorlar["kirill"].Faol(yoqgichlar["kirill_royxat"].Holat);
            if (qatorlar.ContainsKey("organish")) qatorlar["organish"].Faol(yoqgichlar["lugat"].Holat);
            if (tugmalarRasmi != null) tugmalarRasmi.Invalidate();
            SinovniYangila();
        }

        void SinovniYangila()
        {
            if (kirish == null) return;
            uint b = 0;
            if (Reestr.Oqi("lotin")) b |= 1;
            if (Reestr.Oqi("kirill")) b |= 2;
            if (Reestr.Oqi("sh_ajrat")) b |= 4;
            if (Reestr.Oqi("tutuq")) b |= 8;
            if (Reestr.Oqi("istisno")) b |= 16;
            if (Reestr.Oqi("lugat")) b |= 32;
            string n = Ogirgich.Ogir(kirish.Text, b);
            natija.Text = n ?? "(o'girgich topilmadi)";
        }

        void LugatniYangila()
        {
            if (lugatKarta == null) return;
            lugatKarta.SuspendLayout();
            foreach (Control c in lugatKarta.Controls.Cast<Control>().ToList()) { lugatKarta.Controls.Remove(c); c.Dispose(); }
            var yangi = new TextBox { BorderStyle = BorderStyle.FixedSingle, Font = Mavzu.F(10.5f), BackColor = Mavzu.Karta, ForeColor = Mavzu.Matn, Width = Mavzu.D(260) };
            Action qosh = () =>
            {
                string soz = yangi.Text.Trim();
                if (soz.Length < 2) return;
                var sozlar = Lugat.Sozlar();
                if (!sozlar.Contains(soz, StringComparer.OrdinalIgnoreCase)) Lugat.Yoz(sozlar.Concat(new[] { soz }));
                Xabar("«" + soz + "» lug'atga qo'shildi: endi o'girilmaydi.");
                LugatniYangila();
            };
            yangi.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { qosh(); e.SuppressKeyPress = true; } };
            var qator = new Qator("", "So'z qo'shish", "masalan, Photoshop", null);
            qator.Controls.Add(yangi);
            var tugma = Tugma("Qo'shish", qosh);
            qator.Controls.Add(tugma);
            qator.Resize += (s, e) =>
            {
                tugma.Location = new Point(qator.Width - tugma.Width - Mavzu.D(18), (qator.Height - tugma.Height) / 2);
                yangi.Location = new Point(tugma.Left - yangi.Width - Mavzu.D(10), (qator.Height - yangi.Height) / 2);
            };
            lugatKarta.Controls.Add(qator);
            var sozlar2 = Lugat.Sozlar();
            foreach (var soz in sozlar2)
            {
                string s2 = soz;
                lugatKarta.Controls.Add(new Qator("", soz, null, Tugma("Olib tashlash", () =>
                {
                    Lugat.Yoz(Lugat.Sozlar().Where(x => !string.Equals(x, s2, StringComparison.OrdinalIgnoreCase)));
                    Xabar("«" + s2 + "» lug'atdan olindi: endi odatdagidek o'giriladi.");
                    LugatniYangila();
                })));
            }
            if (sozlar2.Count == 0) lugatKarta.Controls.Add(new Qator(null, "Lug'at bo'sh", "O'zingiz yoki Backspace orqali qo'shiladi", null));
            lugatKarta.ResumeLayout();
            Joyla();
            SinovniYangila();
        }

        void BannerniYangila()
        {
            bool kerak = !Tillar.Bormi(Tillar.Oz);
            if (banner.Visible != kerak) { banner.Visible = kerak; Joyla(); }
        }

        void Xabar(string matn)
        {
            bildirish.Text = matn;
            bildirish.Visible = true;
            BildirishniJoyla();
            bildirish.BringToFront();
            bildirishVaqti.Stop();
            bildirishVaqti.Start();
        }

        void BildirishniJoyla()
        {
            int en = Math.Min(ClientSize.Width - Mavzu.D(40), TextRenderer.MeasureText(bildirish.Text, bildirish.Font).Width + Mavzu.D(48));
            bildirish.SetBounds((ClientSize.Width - en) / 2, ClientSize.Height - Mavzu.D(64), en, Mavzu.D(40));
        }

        static void Och(string manzil)
        {
            try { Process.Start(new ProcessStartInfo(manzil) { UseShellExecute = true }); } catch { }
        }

        // -- sinov uchun surat --

        public void Skrin(string yol)
        {
            int jami = ustun.Where(c => c.Visible).Sum(c => c.Height + Mavzu.D(14)) + Mavzu.D(60);
            ClientSize = new Size(ClientSize.Width, jami);
            Joyla();
            using (var bmp = new Bitmap(ClientSize.Width, ClientSize.Height))
            {
                DrawToBitmap(bmp, new Rectangle(Point.Empty, ClientSize));
                bmp.Save(yol, System.Drawing.Imaging.ImageFormat.Png);
            }
        }
    }

    static class Dastur
    {
        [DllImport("user32.dll")] static extern bool SetProcessDpiAwarenessContext(IntPtr qiymat);
        [DllImport("user32.dll")] static extern bool SetProcessDPIAware();

        [STAThread]
        static void Main(string[] args)
        {
            try { if (!SetProcessDpiAwarenessContext(new IntPtr(-2))) SetProcessDPIAware(); }  // SYSTEM_AWARE
            catch { try { SetProcessDPIAware(); } catch { } }
            using (var g = Graphics.FromHwnd(IntPtr.Zero)) Mavzu.K = g.DpiX / 96f;
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Mavzu.Aniqla();
            var oyna = new Oyna();
            int i = Array.IndexOf(args, "--skrin");
            if (i >= 0 && i + 1 < args.Length)
            {
                string yol = args[i + 1];
                oyna.Shown += (s, e) => { oyna.Skrin(yol); oyna.Close(); };
            }
            Application.Run(oyna);
        }
    }
}
