using System;
using System.Collections;
using System.IO;

namespace PSFExtractor
{
    class Program
    {
        static int Main(string[] args)
        {
            Console.WriteLine("PSFExtractor v1.07 - 2021/8/6\nBy th1r5bvn23: https://www.betaworld.cn/\n");
            if (args.Length != 1)
            {
                PrintHelp();
                return 0;
            }
            string CABFileName = args[0];
            if (!File.Exists(CABFileName))
            {
                PrintError(1);
                return 1;
            }
            string DirectoryName = CABFileName.Substring(0, CABFileName.LastIndexOf('.'));
            try
            {
                Directory.CreateDirectory(DirectoryName);
            }
            catch(Exception e)
            {
                PrintError(2);
                return 1;
            }
            string PSFFileName = DirectoryName + ".psf";
            if (!File.Exists(PSFFileName))
            {
                PrintError(3);
                return 1;
            }
            try
            {
                PreProcessing.PreProcess.Process(CABFileName, DirectoryName);
            }
            catch(Exception e)
            {
                PrintError(4);
                return 1;
            }
            SplitPSF.DeltaFileList.List = new ArrayList();
            try
            {
                SplitPSF.GenerateFileList.Generate(PSFFileName, DirectoryName);
            }
            catch(Exception e)
            {
                PrintError(5);
                return 1;
            }
            try
            {
                SplitPSF.SplitOutput.WriteOutput(PSFFileName, DirectoryName);
            }
            catch(Exception e)
            {
                Console.WriteLine(e.ToString());
                PrintError(6);
                return 1;
            }
            PrintError(0);
            return 0;
        }

        static void PrintHelp()
        {
            //                |---------------------------------------80---------------------------------------|
            // Won't implement these features until v1.1
            /*
            Console.WriteLine("Usage: PSFExtractor.exe <CAB file>\n" +
                              "       PSFExtractor.exe --manual <PSF file> <XML file> [--prefer <type>]\n\n" +
                              "    Auto mode      Auto detect CAB file and PSF file. Works only for Windows 10+.\n" +
                              "    Manual mode    Specify PSF file and its descriptive XML file manually. Works\n" +
                              "                   for any update.\n" +
                              "    <CAB file>     Path to CAB file. Use only in auto mode. Need corresponding\n" +
                              "                   PSF file with the same name in the same location.\n" +
                              "    <PSF file>     Path to PSF file. Use only in manual mode.\n" +
                              "    <XML file>     Path to XML file. Use only in manual mode.\n" +
                              "    <type>         Specify the preferred type when multiple source is available.\n" +
                              "                   Default value is RAW.\n");
            */
            Console.WriteLine("Usage: PSFExtractor.exe <CAB file>\n");
        }

        static void PrintError(int ErrorCode)
        {
            switch (ErrorCode)
            {
                case 0:
                    Console.WriteLine("Finished!");
                    break;
                case 1:
                    Console.WriteLine("Error: no CAB file.");
                    break;
                case 2:
                    Console.WriteLine("Error: failed to create directory.");
                    break;
                case 3:
                    Console.WriteLine("Error: no corresponding PSF file.");
                    break;
                case 4:
                    Console.WriteLine("Error: expand failed.");
                    break;
                case 5:
                    Console.WriteLine("Error: invalid CAB file.");
                    break;
                case 6:
                    Console.WriteLine("Error: failed to write output file.");
                    break;
            }
        }
    }
}
