using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Web.WebView2.WinForms;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Microsoft.Win32;

namespace LauncherWeb
{
    public partial class Form1 : Form
    {
        private WebView2 webView;
        private static readonly HttpClient httpClient = new HttpClient();

        private string _clientFullUrl;
        private string _apiBase;
        private string _updaterArgument;
        private string _launcherExeName;

        private const string CONFIG_PASSWORD = "4252021Fer";

        public Form1()
        {
            this.FormBorderStyle = FormBorderStyle.None;
            this.Text = "Launcher de Mi Juego";
            this.ClientSize = new Size(1050, 600);
            this.StartPosition = FormStartPosition.CenterScreen;

            webView = new WebView2();
            webView.Dock = DockStyle.Fill;
            this.Controls.Add(webView);

            CargarConfiguracion();

            this.Load += async (s, e) =>
            {
                await webView.EnsureCoreWebView2Async(null);

                string url = _apiBase + "/index.html?v=" + DateTime.Now.Ticks;

                webView.CoreWebView2.WebMessageReceived += async (sender, args) =>
                {
                    string rawMessage = args.TryGetWebMessageAsString();
                    try
                    {
                        var message = JObject.Parse(rawMessage);
                        string action = message["action"]?.ToString();

                        if (action == "actualizarJuego")
                        {
                            await ActualizarJuego();
                            InstalarYEJecutar();
                        }
                        else if (action == "instalarJuego")
                        {
                            InstalarYEJecutar();
                        }
                        else if (action == "generarManifiesto")
                        {
                            GenerarManifiesto();
                        }
                        else if (action == "cerrarLauncher")
                        {
                            this.Invoke(new Action(() => this.Close()));
                        }
                        else if (action == "minimizarLauncher")
                        {
                            this.Invoke(new Action(() => this.WindowState = FormWindowState.Minimized));
                        }
                        else if (action == "aplicarConfig")
                        {
                            int resolution = message["resolution"]?.Value<int>() ?? 2;
                            bool windowMode = message["windowMode"]?.Value<bool>() ?? false;
                            bool sound = message["sound"]?.Value<bool>() ?? true;
                            int volume = message["volume"]?.Value<int>() ?? 10;
                            string language = message["language"]?.ToString() ?? "Spn";

                            using (var key = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(@"Software\Webzen\Mu\Config"))
                            {
                                if (key != null)
                                {
                                    key.SetValue("Resolution", resolution, Microsoft.Win32.RegistryValueKind.DWord);
                                    key.SetValue("WindowMode", windowMode ? 1 : 0, Microsoft.Win32.RegistryValueKind.DWord);
                                    key.SetValue("SoundOnOFF", sound ? 1 : 0, Microsoft.Win32.RegistryValueKind.DWord);
                                    key.SetValue("MusicOnOFF", sound ? 1 : 0, Microsoft.Win32.RegistryValueKind.DWord);
                                    key.SetValue("VolumeLevel", volume, Microsoft.Win32.RegistryValueKind.DWord);
                                    key.SetValue("LangSelection", language, Microsoft.Win32.RegistryValueKind.String);
                                }
                            }
                        }
                        else if (action == "leerConfig")
                        {
                            int resolution = 2;
                            bool windowMode = false;
                            bool sound = true;
                            int volume = 10;
                            string language = "Spn";

                            using (var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(@"Software\Webzen\Mu\Config"))
                            {
                                if (key != null)
                                {
                                    resolution = (int)(key.GetValue("Resolution", 2));
                                    windowMode = (int)(key.GetValue("WindowMode", 0)) == 1;
                                    sound = (int)(key.GetValue("SoundOnOFF", 1)) == 1;
                                    volume = (int)(key.GetValue("VolumeLevel", 10));
                                    language = key.GetValue("LangSelection", "Spn")?.ToString() ?? "Spn";
                                }
                            }

                            webView.CoreWebView2.ExecuteScriptAsync(
                                $"actualizarConfigUI({resolution}, {windowMode.ToString().ToLower()}, {sound.ToString().ToLower()}, {volume}, '{language}')"
                            );
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("Error al procesar mensaje: " + ex.Message);
                    }
                };

                webView.CoreWebView2.Navigate(url);
            };
        }

        // ================ CONFIGURACIÓN ENCRIPTADA ================
        private void CargarConfiguracion()
        {
            string configPath = Path.Combine(Application.StartupPath, "config.dat");
            if (!File.Exists(configPath))
            {
                UsarConfiguracionPorDefecto();
                return;
            }

            try
            {
                byte[] fileBytes = File.ReadAllBytes(configPath);
                string plainText = DecryptString(fileBytes, CONFIG_PASSWORD);
                byte[] decodedBytes = Convert.FromBase64String(plainText);
                string rawText = Encoding.UTF8.GetString(decodedBytes);
                string[] parts = rawText.Split('|');
                if (parts.Length >= 4)
                {
                    _clientFullUrl = parts[0];
                    _apiBase = parts[1];
                    _updaterArgument = parts[2];
                    _launcherExeName = parts[3];
                }
                else
                {
                    UsarConfiguracionPorDefecto();
                }
            }
            catch
            {
                UsarConfiguracionPorDefecto();
            }
        }

        private string DecryptString(byte[] cipherBytes, string password)
        {
            byte[] salt = new byte[16];
            Array.Copy(cipherBytes, 0, salt, 0, 16);
            byte[] encryptedData = new byte[cipherBytes.Length - 16];
            Array.Copy(cipherBytes, 16, encryptedData, 0, encryptedData.Length);

            using (var deriveBytes = new Rfc2898DeriveBytes(password, salt, 10000))
            {
                byte[] key = deriveBytes.GetBytes(32);
                byte[] iv = deriveBytes.GetBytes(16);

                using (Aes aes = Aes.Create())
                {
                    aes.Key = key;
                    aes.IV = iv;
                    using (var decryptor = aes.CreateDecryptor())
                    using (var ms = new MemoryStream(encryptedData))
                    using (var cs = new CryptoStream(ms, decryptor, CryptoStreamMode.Read))
                    using (var sr = new StreamReader(cs))
                    {
                        return sr.ReadToEnd();
                    }
                }
            }
        }

        private void UsarConfiguracionPorDefecto()
        {
            _clientFullUrl = "https://download1500.mediafire.com/...";
            _apiBase = "https://publicador.mudonald.online/launcher";
            _updaterArgument = "Updater";
            _launcherExeName = "Launcher.exe";
        }

        // ================ ACTUALIZACIÓN CON PROGRESO ================
        private async Task ActualizarJuego()
        {
            string manifiestoUrl = _apiBase + "/manifest.json?v=" + DateTime.Now.Ticks;
            string carpetaJuego = Application.StartupPath;

            if (!Directory.Exists(carpetaJuego) || !File.Exists(Path.Combine(carpetaJuego, "version.txt")))
            {
                await DescargarClienteCompleto();
                try
                {
                    string json = await httpClient.GetStringAsync(manifiestoUrl);
                    var manifiesto = JObject.Parse(json);
                    string versionRemota = manifiesto["version"]?.ToString();
                    File.WriteAllText(Path.Combine(carpetaJuego, "version.txt"), versionRemota ?? "1.0.0");
                    webView.CoreWebView2.ExecuteScriptAsync("actualizarProgreso(100, 'Juego instalado correctamente')");
                }
                catch { }
                return;
            }

            try
            {
                string json = await httpClient.GetStringAsync(manifiestoUrl);
                var manifiesto = JObject.Parse(json);
                string versionRemota = manifiesto["version"]?.ToString();
                var archivos = manifiesto["archivos"] as JObject;

                var archivosADescargar = new List<(string rutaRelativa, string shaRemoto, long size)>();
                long totalSizeToDownload = 0;

                foreach (var entrada in archivos)
                {
                    string rutaRelativa = entrada.Key;
                    string shaRemoto = entrada.Value["sha256"]?.ToString();
                    long sizeRemoto = entrada.Value["size"]?.Value<long>() ?? 0;
                    string rutaLocal = Path.Combine(carpetaJuego, rutaRelativa.Replace('/', Path.DirectorySeparatorChar));

                    bool necesita = true;
                    if (File.Exists(rutaLocal))
                    {
                        string shaLocal = CalcularSHA256(rutaLocal);
                        if (shaLocal.Equals(shaRemoto, StringComparison.OrdinalIgnoreCase))
                            necesita = false;
                    }

                    if (necesita)
                    {
                        archivosADescargar.Add((rutaRelativa, shaRemoto, sizeRemoto));
                        totalSizeToDownload += sizeRemoto;
                    }
                }

                webView.CoreWebView2.ExecuteScriptAsync($"setDownloadTotalBytes({totalSizeToDownload})");

                if (archivosADescargar.Count == 0)
                {
                    File.WriteAllText(Path.Combine(carpetaJuego, "version.txt"), versionRemota ?? "desconocida");
                    webView.CoreWebView2.ExecuteScriptAsync("actualizarProgreso(100, 'Juego ya actualizado')");
                    return;
                }

                long bytesDescargados = 0;
                int contador = 0;
                foreach (var (rutaRelativa, shaRemoto, size) in archivosADescargar)
                {
                    string urlArchivo = $"{_apiBase}/descargar_archivo.php?file={Uri.EscapeDataString(rutaRelativa)}";
                    byte[] datos = await httpClient.GetByteArrayAsync(urlArchivo);
                    string rutaLocal = Path.Combine(carpetaJuego, rutaRelativa.Replace('/', Path.DirectorySeparatorChar));
                    Directory.CreateDirectory(Path.GetDirectoryName(rutaLocal));
                    File.WriteAllBytes(rutaLocal, datos);

                    bytesDescargados += size;
                    contador++;
                    int porcentaje = (int)(bytesDescargados * 100 / totalSizeToDownload);
                    string mensaje = $"Descargando {contador}/{archivosADescargar.Count}: {rutaRelativa}";
                    webView.CoreWebView2.ExecuteScriptAsync($"actualizarProgreso({porcentaje}, '{mensaje}')");
                }

                File.WriteAllText(Path.Combine(carpetaJuego, "version.txt"), versionRemota ?? "desconocida");
                webView.CoreWebView2.ExecuteScriptAsync("actualizarProgreso(100, 'Actualización completada')");
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error al actualizar: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        // ================ DESCARGA COMPLETA (CON USER-AGENT) ================
        private async Task DescargarClienteCompleto()
        {
            string destinoZip = Path.Combine(Application.StartupPath, "cliente.zip");
            string carpetaJuego = Application.StartupPath;

            using (var request = new HttpRequestMessage(HttpMethod.Get, _clientFullUrl))
            {
                request.Headers.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
                request.Headers.Add("Accept", "*/*");

                using (var response = await httpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead))
                {
                    response.EnsureSuccessStatusCode();

                    long total = response.Content.Headers.ContentLength ?? -1;
                    if (total > 0)
                        webView.CoreWebView2.ExecuteScriptAsync($"setDownloadTotalBytes({total})");

                    long totalRead = 0;

                    using (var stream = await response.Content.ReadAsStreamAsync())
                    using (var fileStream = new FileStream(destinoZip, FileMode.Create, FileAccess.Write, FileShare.None))
                    {
                        byte[] buffer = new byte[8192];
                        int bytesRead;
                        while ((bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length)) > 0)
                        {
                            await fileStream.WriteAsync(buffer, 0, bytesRead);
                            totalRead += bytesRead;
                            int porcentaje = total > 0 ? (int)(totalRead * 100 / total) : 0;
                            webView.CoreWebView2.ExecuteScriptAsync($"actualizarProgreso({porcentaje}, 'Descargando cliente...')");
                        }
                    }

                    if (total > 0 && totalRead != total)
                    {
                        File.Delete(destinoZip);
                        MessageBox.Show("La descarga se interrumpió. Intentá de nuevo.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }
                }
            }

            webView.CoreWebView2.ExecuteScriptAsync("actualizarProgreso(0, 'Instalando juego...')");
            var progress = new Progress<int>(porcentaje =>
            {
                webView.CoreWebView2.ExecuteScriptAsync($"actualizarProgreso({porcentaje}, 'Instalando... {porcentaje}%')");
            });

            await Task.Run(() => ExtraerZipConProgreso(destinoZip, carpetaJuego, progress));

            File.Delete(destinoZip);
            webView.CoreWebView2.ExecuteScriptAsync("actualizarProgreso(100, 'Instalación completa')");
        }

        // ================ EXTRAER ZIP CON PROGRESO ================
        private void ExtraerZipConProgreso(string zipPath, string destino, IProgress<int> progress)
        {
            using (var archivo = new ZipArchive(File.OpenRead(zipPath), ZipArchiveMode.Read))
            {
                int totalEntradas = archivo.Entries.Count;
                int entradasProcesadas = 0;

                foreach (var entrada in archivo.Entries)
                {
                    string rutaCompleta = Path.Combine(destino, entrada.FullName);
                    Directory.CreateDirectory(Path.GetDirectoryName(rutaCompleta));

                    if (!string.IsNullOrEmpty(Path.GetFileName(rutaCompleta)))
                    {
                        using (var streamEntrada = entrada.Open())
                        using (var streamSalida = File.Create(rutaCompleta))
                        {
                            streamEntrada.CopyTo(streamSalida);
                        }
                    }

                    entradasProcesadas++;
                    int porcentaje = (int)(entradasProcesadas * 100.0 / totalEntradas);
                    progress?.Report(porcentaje);
                }
            }
        }

        // ================ EJECUTAR EL JUEGO ================
        private void InstalarYEJecutar()
        {
            string installDir = Application.StartupPath;
            string launcherExe = Application.ExecutablePath;

            string exePath = Path.Combine(installDir, "main.exe");
            if (!File.Exists(exePath))
            {
                string[] exes = Directory.GetFiles(installDir, "*.exe", SearchOption.TopDirectoryOnly)
                    .Where(e => !e.Equals(launcherExe, StringComparison.OrdinalIgnoreCase))
                    .ToArray();
                if (exes.Length > 0)
                    exePath = exes[0];
                else
                {
                    MessageBox.Show("No se encontró un ejecutable del juego en la carpeta.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }

            string args = $"{_updaterArgument} /launcher:{_launcherExeName}";
            System.Diagnostics.Process.Start(exePath, args);
            this.Invoke(new Action(() => this.WindowState = FormWindowState.Minimized));
        }

        // ================ GENERADOR DE MANIFIESTO (ADMIN) ================
        private void GenerarManifiesto()
        {
            using (var dialog = new FolderBrowserDialog())
            {
                dialog.Description = "Selecciona la carpeta raíz del juego (donde está el ejecutable)";
                if (dialog.ShowDialog() == DialogResult.OK)
                {
                    string carpetaJuego = dialog.SelectedPath;

                    string version = "1.0.0";
                    using (var inputForm = new Form())
                    {
                        inputForm.Text = "Versión del manifiesto";
                        inputForm.ClientSize = new Size(300, 120);
                        inputForm.FormBorderStyle = FormBorderStyle.FixedDialog;
                        inputForm.StartPosition = FormStartPosition.CenterParent;
                        inputForm.MaximizeBox = false;
                        inputForm.MinimizeBox = false;

                        var label = new Label() { Text = "Introduce la versión:", Location = new Point(10, 20), AutoSize = true };
                        var textBox = new TextBox() { Location = new Point(10, 45), Size = new Size(260, 20) };
                        var buttonOk = new Button() { Text = "OK", Location = new Point(100, 75), DialogResult = DialogResult.OK };
                        var buttonCancel = new Button() { Text = "Cancelar", Location = new Point(180, 75), DialogResult = DialogResult.Cancel };

                        inputForm.Controls.Add(label);
                        inputForm.Controls.Add(textBox);
                        inputForm.Controls.Add(buttonOk);
                        inputForm.Controls.Add(buttonCancel);
                        inputForm.AcceptButton = buttonOk;

                        if (inputForm.ShowDialog() == DialogResult.OK && !string.IsNullOrWhiteSpace(textBox.Text))
                            version = textBox.Text.Trim();
                    }

                    var manifiesto = new Dictionary<string, object>();
                    manifiesto["version"] = version;
                    var archivos = new Dictionary<string, object>();

                    foreach (string archivo in Directory.GetFiles(carpetaJuego, "*", SearchOption.AllDirectories))
                    {
                        string rutaRelativa = archivo.Substring(carpetaJuego.Length + 1).Replace('\\', '/').ToLowerInvariant();
                        string sha256 = CalcularSHA256(archivo);
                        long size = new FileInfo(archivo).Length;
                        archivos[rutaRelativa] = new { sha256, size };
                    }

                    manifiesto["archivos"] = archivos;

                    string json = JsonConvert.SerializeObject(manifiesto, Formatting.Indented);
                    string rutaManifiesto = Path.Combine(Application.StartupPath, "manifest_generado.json");
                    File.WriteAllText(rutaManifiesto, json);
                    MessageBox.Show($"Manifiesto generado en:\n{rutaManifiesto}", "Éxito", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
        }

        // ================ CALCULAR SHA256 ================
        private string CalcularSHA256(string ruta)
        {
            using (var sha = SHA256.Create())
            using (var stream = File.OpenRead(ruta))
            {
                byte[] hash = sha.ComputeHash(stream);
                return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
            }
        }
    }
}