using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Newtonsoft.Json;
using System.Globalization;

namespace bandcampdl
{
    public class TralbumData
    {
        public class Current
        {
            public string release_date;
            [JsonIgnore]
            public DateTime release_date_datetime { get { return Program.ParseDateFromJson(release_date); } }
            public string about;
            public string credits;
            public string title;
        }
        public Current current;
        public string artFullsizeUrl;
        public class TrackInfo
        {
            public string has_info;
            public string title;
            public string title_link;
            public string file;
        }
        public TrackInfo[] trackinfo;
        public string artist;
    }
}
