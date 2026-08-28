using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Windows.Forms;

public partial class ConfigForm : Form
{
    const string SECRET_PASSWORD = "4252021Fer";

    private TextBox txtClientUrl;
    private TextBox txtApiBase;
    private TextBox txtUpdaterArg;
    private TextBox txtLauncherName;

    public ConfigForm()
    {
        this.ClientSize = new Size(620, 470);
        this.StartPosition = FormStartPosition.CenterScreen;
        this.FormBorderStyle = FormBorderStyle.None;
        this.BackColor = Color.FromArgb(40, 40, 55);
        this.DoubleBuffered = true;

        Panel titleBar = new Panel()
        {
            Height = 40,
            Dock = DockStyle.Top,
            BackColor = Color.FromArgb(233, 69, 96)
        };
        this.Controls.Add(titleBar);

        Label title = new Label()
        {
            Text = "⚙️ Generador de Configuración",
            ForeColor = Color.White,
            Font = new Font("Segoe UI", 11f, FontStyle.Bold),
            Location = new Point(15, 8),
            AutoSize = true
        };
        titleBar.Controls.Add(title);

        Label closeBtn = new Label()
        {
            Text = "✕",
            ForeColor = Color.White,
            Font = new Font("Segoe UI", 14f, FontStyle.Bold),
            Cursor = Cursors.Hand,
            Anchor = AnchorStyles.Top | AnchorStyles.Right,
            Location = new Point(titleBar.Width - 35, 5),
            AutoSize = true
        };
        closeBtn.Click += (s, e) => this.Close();
        titleBar.Controls.Add(closeBtn);

        Panel formPanel = new Panel()
        {
            Dock = DockStyle.Fill,
            BackColor = Color.White,
            Padding = new Padding(25)
        };
        this.Controls.Add(formPanel);

        int y = 50;
        int marginLeft = 10;

        AddLabel(formPanel, "URL del cliente completo:", y, marginLeft, true); y += 24;
        txtClientUrl = AddTextBox(formPanel, y, marginLeft); y += 48;
        AddLabel(formPanel, "API Base (ej: https://.../launcher):", y, marginLeft, true); y += 24;
        txtApiBase = AddTextBox(formPanel, y, marginLeft); y += 48;
        AddLabel(formPanel, "Argumento de actualización:", y, marginLeft, true); y += 24;
        txtUpdaterArg = AddTextBox(formPanel, y, marginLeft); txtUpdaterArg.Text = "Updater"; y += 48;
        AddLabel(formPanel, "Nombre del launcher (.exe):", y, marginLeft, true); y += 24;
        txtLauncherName = AddTextBox(formPanel, y, marginLeft); txtLauncherName.Text = "Launcher.exe"; y += 58;

        Button btnGenerate = new Button()
        {
            Text = "📦 Generar config.dat",
            BackColor = Color.FromArgb(233, 69, 96),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Cursor = Cursors.Hand,
            Location = new Point((formPanel.Width - 220) / 2, y),
            Size = new Size(220, 42)
        };
        btnGenerate.FlatAppearance.BorderSize = 0;
        btnGenerate.Region = Region.FromHrgn(CreateRoundRectRgn(0, 0, btnGenerate.Width, btnGenerate.Height, 8, 8));
        btnGenerate.Click += (s, e) => GenerarConfig();
        formPanel.Controls.Add(btnGenerate);
    }

    private void AddLabel(Panel parent, string text, int top, int left, bool bold = false)
    {
        var lbl = new Label()
        {
            Text = text,
            Location = new Point(left, top),
            AutoSize = true,
            ForeColor = Color.FromArgb(80, 80, 80),
            Font = bold ? new Font("Segoe UI", 9f, FontStyle.Bold) : new Font("Segoe UI", 9f)
        };
        parent.Controls.Add(lbl);
    }

    private TextBox AddTextBox(Panel parent, int top, int left)
    {
        var tb = new TextBox()
        {
            Location = new Point(left, top),
            Size = new Size(parent.Width - (left * 2), 28),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.FromArgb(250, 250, 250),
            Font = new Font("Segoe UI", 9f)
        };
        parent.Controls.Add(tb);
        return tb;
    }

    private void GenerarConfig()
    {
        string clientUrl = txtClientUrl.Text.Trim();
        string apiBase = txtApiBase.Text.Trim();
        string updaterArg = txtUpdaterArg.Text.Trim();
        string launcherName = txtLauncherName.Text.Trim();

        if (string.IsNullOrEmpty(clientUrl) || string.IsNullOrEmpty(apiBase) ||
            string.IsNullOrEmpty(updaterArg) || string.IsNullOrEmpty(launcherName))
        {
            MessageBox.Show("Completá todos los campos.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        // 1. Construir cadena original
        string rawText = $"{clientUrl}|{apiBase}|{updaterArg}|{launcherName}";
        // 2. Convertir a Base64 para encapsular caracteres especiales
        string plainText = Convert.ToBase64String(Encoding.UTF8.GetBytes(rawText));

        try
        {
            byte[] encrypted = EncryptString(plainText, SECRET_PASSWORD);

            using (var sfd = new SaveFileDialog())
            {
                sfd.FileName = "config.dat";
                sfd.Filter = "Archivo encriptado (*.dat)|*.dat";
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllBytes(sfd.FileName, encrypted);
                    MessageBox.Show("✅ Archivo generado.\nCopialo a la carpeta del launcher.", "Listo", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show("Error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    static byte[] EncryptString(string plainText, string password)
    {
        using (var deriveBytes = new Rfc2898DeriveBytes(password, 16, 10000))
        {
            byte[] salt = deriveBytes.Salt;
            byte[] key = deriveBytes.GetBytes(32);
            byte[] iv = deriveBytes.GetBytes(16);

            using (Aes aes = Aes.Create())
            {
                aes.Key = key;
                aes.IV = iv;
                using (var encryptor = aes.CreateEncryptor())
                using (var ms = new MemoryStream())
                {
                    ms.Write(salt, 0, salt.Length);
                    using (var cs = new CryptoStream(ms, encryptor, CryptoStreamMode.Write))
                    using (var sw = new StreamWriter(cs))
                    {
                        sw.Write(plainText);
                    }
                    return ms.ToArray();
                }
            }
        }
    }

    [System.Runtime.InteropServices.DllImport("Gdi32.dll")]
    private static extern IntPtr CreateRoundRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nWidthEllipse, int nHeightEllipse);
}