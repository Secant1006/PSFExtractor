using System.IO;
using System.Globalization;

namespace PSFExtractor.SplitPSF
{
    class GenerateFileListV1
    {
        public static void Generate(string PSMFile)
        {
            string[] Lines = File.ReadAllLines(PSMFile);
            bool IsReadingAFile = false;
            string FileName = null;
            string sourceType;
            long sourceOffset;
            int sourceLength;
            foreach (string Line in Lines)
            {
                if (IsReadingAFile)
                {
                    if (Line.Contains("p0="))
                    {
                        sourceType = "PA19";
                    }
                    else if (Line.Contains("full="))
                    {
                        sourceType = "FULL";
                    }
                    else
                    {
                        throw new IOException();
                    }
                    sourceOffset = long.Parse(Line.Substring(Line.LastIndexOf('=') + 1, Line.LastIndexOf(',') - Line.LastIndexOf('=') - 1), NumberStyles.HexNumber);
                    sourceLength = int.Parse(Line.Substring(Line.LastIndexOf(',') + 1), NumberStyles.HexNumber);
                    DeltaFileList.List.Add(new DeltaFile(FileName, 0, sourceType, sourceOffset, sourceLength));
                    IsReadingAFile = false;
                    continue;
                }
                if (Line.Length > 0)
                {
                    if (Line[0].Equals('['))
                    {
                        FileName = Line.Substring(1, Line.Length - 2);
                        IsReadingAFile = true;
                    }
                }
            }
        }
    }
}
