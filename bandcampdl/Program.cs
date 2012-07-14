using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.IO;
using Newtonsoft.Json;

namespace bandcampdl
{
    public static class Program
    {
        public static void Main(string[] args)
        {
            WebClient client = new WebClient();
            foreach (string album in args)
            {
                Console.WriteLine("Retrieving album info..." /* from " + album + "..."*/);
                HttpWebRequest req = (HttpWebRequest)HttpWebRequest.Create(album);
                HttpWebResponse resp = (HttpWebResponse)req.GetResponse();
                Stream dataStream = resp.GetResponseStream();
                StreamReader reader = new StreamReader(dataStream);
                List<string> page = new List<string>();
                while (!reader.EndOfStream)
                    page.Add(reader.ReadLine());
                reader.Close();
                dataStream.Close();
                resp.Close();
                int albumdatastart = page.IndexOf("var TralbumData = {");
                int albumdataend = page.IndexOf("};", albumdatastart);
                string jsondata = "{";
                for (int i = albumdatastart + 4; i < albumdataend; i++)
                    jsondata += page[i];
                jsondata += " }";
                StringReader sr = new StringReader(jsondata);
                JsonTextReader jr = new JsonTextReader(sr);
                JsonSerializer js = new JsonSerializer();
                TralbumData data = js.Deserialize<TralbumData>(jr);
                jr.Close();
                sr.Close();
                string albumPath = data.current.title;
                foreach (char item in Path.GetInvalidPathChars())
                    albumPath = albumPath.Replace(item, '_');
                albumPath = albumPath.Replace(": ", " - ");
                albumPath = Path.Combine(Environment.CurrentDirectory, albumPath);
                Directory.CreateDirectory(albumPath);
                Console.WriteLine("Found " + data.trackinfo.Length + " tracks in album \"" + data.current.title + "\"...");
                client.DownloadFile(data.artFullsizeUrl, Path.Combine(albumPath, "art.png"));
                string hostname = new Uri(album).Host;
                for (int i = 0; i < data.trackinfo.Length; i++)
                {
                    Console.Write("Downloading track \"" + data.trackinfo[i].title + "\" (" + (i + 1) + "/" + data.trackinfo.Length + ")...");
                    string filePath = data.trackinfo[i].title;
                    if (data.trackinfo.Length < 100)
                    {
                        filePath = (i + 1).ToString("D2") + " - " + filePath;
                    }
                    else
                    {
                        filePath = (i + 1).ToString("D3") + " - " + filePath;
                    }
                    foreach (char item in Path.GetInvalidFileNameChars())
                        filePath = filePath.Replace(item, '_');
                    filePath = filePath.Replace(": ", " - ").Replace('/', '-').Replace('\\', '-');
                    filePath = Path.ChangeExtension(Path.Combine(albumPath, filePath), "mp3");
                    client.DownloadFile(data.trackinfo[i].file, filePath);
                    IdSharp.Tagging.ID3v2.ID3v2Tag tag = new IdSharp.Tagging.ID3v2.ID3v2Tag(filePath);
                    tag.Album = data.current.title;
                    tag.AlbumArtist = tag.Artist = data.artist;
                    IdSharp.Tagging.ID3v2.Frames.IAttachedPicture pic = tag.PictureList.AddNew();
                    pic.Picture = System.Drawing.Image.FromFile(Path.Combine(albumPath, "art.png"));
                    if (data.trackinfo[i].has_info != null)
                    {
                        IdSharp.Tagging.ID3v2.Frames.IComments com = tag.CommentsList.AddNew();
                        com.Value = data.trackinfo[i].has_info;
                    }
                    tag.ReleaseTimestamp = data.current.release_date_datetime.ToString("s", System.Globalization.DateTimeFormatInfo.InvariantInfo);
                    tag.Title = data.trackinfo[i].title;
                    tag.TrackNumber = (i + 1).ToString(System.Globalization.NumberFormatInfo.InvariantInfo);
                    tag.Year = data.current.release_date_datetime.Year.ToString("0000", System.Globalization.NumberFormatInfo.InvariantInfo);
                    tag.CommercialInfoUrlList.AddNew().Value = album;
                    tag.AudioFileUrl = "http://" + hostname + data.trackinfo[i].title_link;
                    tag.ArtistUrlList.AddNew().Value = "http://" + hostname + "/";
                    tag.Save(filePath);
                    Console.WriteLine(" downloaded!");
                }
                File.WriteAllText(Path.Combine(albumPath, "Info.txt"), data.current.title + " by " + data.artist + "\r\nReleased " + data.current.release_date_datetime.ToString() + "\r\n" + data.current.about + "\r\n" + data.current.credits);
            }
        }

        public static DateTime ParseDateFromJson(string date)
        {
            return DateTime.ParseExact(date, "ddd MMM dd HH:mm:ss UTC yyyy", System.Globalization.DateTimeFormatInfo.InvariantInfo, System.Globalization.DateTimeStyles.AssumeUniversal).ToUniversalTime();
        }
    }
}
