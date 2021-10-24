using System;
using System.Xml;

namespace PSFExtractor.SplitPSF
{
    class GenerateFileList
    {
        public static void Generate(string XMLFileName)
        {
            Console.Write("Reading file info...");
            XmlDocument doc = new XmlDocument();
            doc.Load(XMLFileName);
            XmlNode root = doc.FirstChild.NextSibling;
            XmlNode child = root.FirstChild;
            while (!child.LocalName.Equals("Files"))
            {
                child = child.NextSibling;
            }
            XmlNodeList FileList = child.ChildNodes;
            foreach(XmlNode file in FileList)
            {
                XmlElement fileElement = (XmlElement)file;
                string name = fileElement.GetAttribute("name");
                long time = long.Parse(fileElement.GetAttribute("time"));
                XmlNode fileChild = file.FirstChild;
                while (!fileChild.LocalName.Equals("Delta"))
                {
                    fileChild = fileChild.NextSibling;
                }
                XmlNode deltaChild = fileChild.FirstChild;
                while (!deltaChild.LocalName.Equals("Source"))
                {
                    deltaChild = deltaChild.NextSibling;
                }
                XmlElement sourceElement = (XmlElement)deltaChild;
                string sourceType = sourceElement.GetAttribute("type");
                long sourceOffset = long.Parse(sourceElement.GetAttribute("offset"));
                int sourceLength = int.Parse(sourceElement.GetAttribute("length"));
                DeltaFileList.List.Add(new DeltaFile(name, time, sourceType, sourceOffset, sourceLength));
            }
            Console.WriteLine(" OK");
        }
    }
}
