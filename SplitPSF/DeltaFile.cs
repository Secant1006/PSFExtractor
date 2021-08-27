namespace PSFExtractor.SplitPSF
{
    class DeltaFile
    {

        public string FileName;
        public long time;
        public string sourceType;
        public long sourceOffset;
        public int sourceLength;

        public DeltaFile(string FileName, long time, string sourceType, long sourceOffset, int sourceLength)
        {
            this.FileName = FileName;
            this.time = time;
            this.sourceType = sourceType;
            this.sourceOffset = sourceOffset;
            this.sourceLength = sourceLength;
        }
    }
}
