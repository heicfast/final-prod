// rpbuf_test.csx - synthetic RETRIEVAL_POINTERS_BUFFER (true 8-aligned layout)
// parsed with BOTH the old (shifted) and new offsets to prove the fix decodes
// real-world values. Run: dotnet-script or csc; here compiled via mcs/pwsh.
using System;
using System.Collections.Generic;
using System.Text;

class RpbufTest
{
    // Build the buffer the way the KERNEL does: ExtentCount u32 @0, pad @4,
    // StartingVcn i64 @8, per extent @16+16i: NextVcn i64, Lcn i64.
    static byte[] Build(long startingVcn, (long nextVcn, long lcn)[] extents)
    {
        byte[] b = new byte[16 + 16 * extents.Length];
        BitConverter.GetBytes(extents.Length).CopyTo(b, 0);
        BitConverter.GetBytes(startingVcn).CopyTo(b, 8);
        for (int i = 0; i < extents.Length; i++)
        {
            BitConverter.GetBytes(extents[i].nextVcn).CopyTo(b, 16 + 16 * i);
            BitConverter.GetBytes(extents[i].lcn).CopyTo(b, 16 + 16 * i + 8);
        }
        return b;
    }

    static long[][] ParseNew(byte[] outBuf)
    {
        int count = BitConverter.ToInt32(outBuf, 0);
        long prevVcn = BitConverter.ToInt64(outBuf, 8);
        var list = new List<long[]>();
        for (int i = 0; i < count; i++)
        {
            long nextVcn = BitConverter.ToInt64(outBuf, 16 + 16 * i);
            long lcn = BitConverter.ToInt64(outBuf, 16 + 16 * i + 8);
            if (lcn >= 0)
                list.Add(new long[] { lcn, nextVcn - prevVcn });
            prevVcn = nextVcn;
        }
        return list.ToArray();
    }

    static long[][] ParseOld(byte[] outBuf)
    {
        int count = BitConverter.ToInt32(outBuf, 0);
        long prevVcn = BitConverter.ToInt64(outBuf, 4);
        var list = new List<long[]>();
        for (int i = 0; i < count; i++)
        {
            long nextVcn = BitConverter.ToInt64(outBuf, 12 + 16 * i);
            long lcn = BitConverter.ToInt64(outBuf, 12 + 16 * i + 8);
            if (lcn >= 0)
                list.Add(new long[] { lcn, nextVcn - prevVcn });
            prevVcn = nextVcn;
        }
        return list.ToArray();
    }

    static string Show(long[][] e)
    {
        var sb = new StringBuilder();
        foreach (var x in e) sb.Append($"[{x[0]},{x[1]}] ");
        return sb.ToString();
    }

    static int fails = 0;
    static void Check(string name, bool ok)
    {
        Console.WriteLine($"{(ok ? "PASS" : "FAIL")} {name}");
        if (!ok) fails++;
    }

    static void Main()
    {
        // Case 1: the t000.jpg shape - 1 extent, 1 cluster, LCN 5953
        var b1 = Build(0, new[] { (1L, 5953L) });
        var n1 = ParseNew(b1);
        Check($"new parse single extent = [5953,1] (got {Show(n1)})",
              n1.Length == 1 && n1[0][0] == 5953 && n1[0][1] == 1);
        var o1 = ParseOld(b1);
        Console.WriteLine($"  (old parse produced {Show(o1)} - the 2^32 garbage)");

        // Case 2: fragmented - 3 extents, sparse hole (LCN -1 skipped)
        var b2 = Build(0, new[] { (10L, 100L), (20L, -1L), (45L, 700L) });
        var n2 = ParseNew(b2);
        Check($"new parse 3 extents -> 2 real [100,10],[700,25] (got {Show(n2)})",
              n2.Length == 2 && n2[0][0] == 100 && n2[0][1] == 10 && n2[1][0] == 700 && n2[1][1] == 25);

        // Case 3: nonzero StartingVcn (large fragmented file, resumed mapping)
        var b3 = Build(150000L, new[] { (150010L, 999999L) });
        var n3 = ParseNew(b3);
        Check($"new parse StartingVcn=150000 -> len 10 (got {Show(n3)})",
              n3.Length == 1 && n3[0][0] == 999999 && n3[0][1] == 10);

        // Case 4: the observed f00000 garbage reproduces under OLD parse
        var b4 = Build(0, new[] { (1L, 5952L) });
        var o4 = ParseOld(b4);
        Check($"old parse reproduces the observed 2^32-length garbage (got {Show(o4)})",
              o4.Length == 1 && o4[0][1] == 4294967296L && o4[0][0] == 5952L << 32);

        Console.WriteLine(fails == 0 ? "ALL PASS" : $"{fails} FAILURES");
        Environment.Exit(fails);
    }
}
