using System;
using System.IO;
using System.Diagnostics;

namespace PSFExtractor.SplitPSF
{
    class SplitOutput
    {

        public static void WriteOutput(string PSFFile, string DirectoryName)
        {
            Console.Write("Writing: " + DeltaFileList.List.Count + " files...");
            FileStream PSFFileStream = File.OpenRead(PSFFile);
            string TempDirectory = Path.GetTempPath() + "PSFExtractor";
            if (!Directory.Exists(TempDirectory))
            {
                Directory.CreateDirectory(TempDirectory);
            }
            foreach (DeltaFile file in DeltaFileList.List)
            {
                string FullFileName = Path.GetFullPath(DirectoryName) + Path.DirectorySeparatorChar + file.FileName;
                string FileName = Path.GetFileName(FullFileName);
                string FullPath = FullFileName.Substring(0, FullFileName.LastIndexOf(Path.DirectorySeparatorChar));
                if (File.Exists(FullFileName))
                {
                    continue;
                }
                bool LengthExceedsLimit = FullFileName.Length > 255 || FullPath.Length > 248;
                string TempFileName = TempDirectory + Path.DirectorySeparatorChar + FileName;
                string UsingFileName = LengthExceedsLimit ? TempFileName : FullFileName;
                if (!LengthExceedsLimit)
                {
                    Directory.CreateDirectory(FullPath);
                }
                PSFFileStream.Seek(file.sourceOffset, SeekOrigin.Begin);
                byte[] Buffer = new byte[file.sourceLength];
                PSFFileStream.Read(Buffer, 0, file.sourceLength);
                FileStream OutputFileStream = File.Create(UsingFileName);
                OutputFileStream.Write(Buffer, 0, file.sourceLength);
                OutputFileStream.Close();
                // New implementation using ApplyDeltaW() from msdelta.dll (thanks to @abbodi1406)
                if (file.sourceType.Equals("PA30", StringComparison.OrdinalIgnoreCase))
                {
                    if (!NativeMethods.ApplyDeltaW(0, null, Path.GetFullPath(UsingFileName), Path.GetFullPath(UsingFileName) + "_PA30"))
                    {
                        throw new IOException();
                    }
                    File.Delete(UsingFileName);
                    File.Move(UsingFileName + "_PA30", UsingFileName);
                }
                File.SetLastWriteTimeUtc(UsingFileName, DateTime.FromFileTimeUtc(file.time));
                if (LengthExceedsLimit)
                {
                    using (Process process = new Process())
                    {
                        process.StartInfo.UseShellExecute = false;
                        process.StartInfo.CreateNoWindow = true;
                        process.StartInfo.FileName = "robocopy.exe";
                        process.StartInfo.Arguments = "\"" + TempDirectory + "\" \"" + FullPath + "\" \"" + FileName + "\" /MOV /R:1 /W:1 /NS /NC /NFL /NDL /NP /NJH /NJS";
                        process.Start();
                        process.WaitForExit();
                        if (process.ExitCode != 0 && process.ExitCode != 1)
                        {
                            throw new IOException();
                        }
                    }
                }
            }
            if (Directory.Exists(TempDirectory))
            {
                Directory.Delete(TempDirectory, true);
            }
            Console.WriteLine(" OK");
        }
    }
}
