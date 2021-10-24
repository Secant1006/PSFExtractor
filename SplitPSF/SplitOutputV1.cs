using System;
using System.IO;

namespace PSFExtractor.SplitPSF
{
    class SplitOutputV1
    {

        public static void WriteOutput(string PSFFile, string DirectoryName)
        {
            Console.Write("Writing: " + DeltaFileList.List.Count + " files...");
            FileStream PSFFileStream = File.OpenRead(PSFFile);
            foreach (DeltaFile file in DeltaFileList.List)
            {
                string FullFileName = Path.GetFullPath(DirectoryName) + Path.DirectorySeparatorChar + file.FileName;
                string FileName = Path.GetFileName(FullFileName);
                string FullPath = FullFileName.Substring(0, FullFileName.LastIndexOf(Path.DirectorySeparatorChar));
                if (File.Exists(FullFileName))
                {
                    continue;
                }
                Directory.CreateDirectory(FullPath);
                PSFFileStream.Seek(file.sourceOffset, SeekOrigin.Begin);
                byte[] Buffer = new byte[file.sourceLength];
                PSFFileStream.Read(Buffer, 0, file.sourceLength);
                FileStream OutputFileStream = File.Create(FullFileName);
                OutputFileStream.Write(Buffer, 0, file.sourceLength);
                OutputFileStream.Close();
                if (file.sourceType.Equals("PA19", StringComparison.OrdinalIgnoreCase))
                {
                    if (!NativeMethods.ApplyDeltaW(1, null, Path.GetFullPath(FullFileName), Path.GetFullPath(FullFileName) + "_PA19"))
                    {
                        throw new IOException();
                    }
                    File.Delete(FullFileName);
                    File.Move(FullFileName + "_PA19", FullFileName);
                }
            }
            Console.WriteLine(" OK");
        }
    }
}
