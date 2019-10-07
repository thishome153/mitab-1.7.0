/**********************************************************************
 * $Id: tabdump.cpp,v 1.16 2006/11/20 20:05:58 dmorissette Exp $
 *
 * Name:     tabdump.cpp
 * Project:  MapInfo TAB format Read/Write library
 * Language: C++
 * Purpose:  Test various part of the lib., and generate binary dumps.
 * Author:   Daniel Morissette, dmorissette@dmsolutions.ca
 *
 **********************************************************************
 * Copyright (c) 1999-2001, Daniel Morissette
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log: tabdump.cpp,v $
 * Revision 1.16  2006/11/20 20:05:58  dmorissette
 * First pass at improving generation of spatial index in .map file (bug 1585)
 * New methods for insertion and splittung in the spatial index are done.
 * Also implemented a method to dump the spatial index to .mif/.mid
 * Still need to implement splitting of TABMapObjectBlock to get optimal
 * results.
 *
 * Revision 1.15  2006/09/05 23:05:51  dmorissette
 * Added -spatialindex option to call TABMAPFile::DumpSpatialIndex() (bug 1585)
 *
 * Revision 1.14  2005/03/22 23:24:54  dmorissette
 * Added support for datum id in .MAP header (bug 910)
 *
 * Revision 1.13  2004/06/30 20:29:04  dmorissette
 * Fixed refs to old address danmo@videotron.ca
 *
 * Revision 1.12  2001/11/16 20:55:00  daniel
 * Added -d option: DumpMapFileBlockDetails()
 *
 * Revision 1.11  2001/09/19 14:33:19  warmerda
 * support mif files for dumping with -all
 *
 * Revision 1.10  2001/09/17 19:52:50  daniel
 * Added DumpViaSpatialIndex()
 *
 * Revision 1.9  2001/07/04 14:13:24  daniel
 * Added GetExtent() output in DumpCoordsys()
 *
 * Revision 1.8  2001/01/23 21:23:42  daniel
 * Added projection bounds lookup table, called from TABFile::SetProjInfo()
 *
 * Revision 1.7  2000/11/13 22:05:45  daniel
 * Added SearchIndex() - For string indexes only
 *
 * Revision 1.6  2000/10/18 03:57:55  daniel
 * Added DumpCoordsys()
 *
 * Revision 1.5  2000/02/28 17:14:31  daniel
 * Report indexed fields
 *
 * Revision 1.4  2000/01/15 22:30:45  daniel
 * Switch to MIT/X-Consortium OpenSource license
 *
 * Revision 1.3  1999/12/14 02:24:41  daniel
 * Use IMapInfoFile::SmartOpen()
 *
 * Revision 1.2  1999/09/20 14:27:49  warmerda
 * Use access of "rb" instead of "r".
 *
 * Revision 1.1  1999/07/12 04:18:26  daniel
 * Initial checkin
 *
 **********************************************************************/


#include "mitab.h"
#include "mitab_utils.h"
#include <ctype.h>

static int DumpMapFileBlocks(const char *pszFname);
static int DumpMapFileObjects(const char *pszFname);
static int DumpMapFileIndexTree2MIF(const char *pszFname, int nMaxDepth);
static int DumpMapFileBlockDetails(const char *pszFname, int nOffset);

static int DumpIndFileObjects(const char *pszFname);

static int DumpTabFile(const char *pszFname);
static int DumpCoordsys(const char *pszFname);
static int DumpCoordsysStruct(const char *pszFname);

static int SearchIndex(const char *pszFname, int nIndexNo, const char *pszVal);

static int DumpViaSpatialIndex(const char *pszFname, 
                               double dXMin, double dYMin, 
                               double dXMax, double dYMax);

#define TABTEST_USAGE \
  "Usage: tabdump -a|-all     <filename>\n" \
  "       tabdump -b|-blocks  <filename>\n" \
  "       tabdump -d|-details <filename> <block_offset>\n" \
  "       tabdump -o|-objects <filename>\n" \
  "       tabdump -i|-index   <filename> <indexno> <val>\n" \
  "       tabdump -s|-spatialindex <filename> [maxdepth]\n" \
  "       tabdump -e|-envelope <filename> <xmin> <ymin> <ymax> <ymax>\n" \
  "       tabdump -c|-coordsys <filename>\n" \
  "       tabdump -Cc|-Ccoordsys <filename>\n"

/**********************************************************************
 *                          main()
 *
 * This program is used to dump binary files (default behavior), and to
 * test some parts of the lib. during the development process. 
 **********************************************************************/
int main(int argc, char *argv[])
{
    const char  *pszFname;

/*---------------------------------------------------------------------
 *      Read program arguments.
 *--------------------------------------------------------------------*/
    if (argc<3)
    {
        printf("%s", TABTEST_USAGE);
        return 1;
    }
    else
    {
        pszFname = argv[2];
    }
    
/*---------------------------------------------------------------------
 *      With option -blocks <filename>
 *      Open file, and dump each block sequentially.
 *--------------------------------------------------------------------*/
    if (EQUALN(argv[1], "-blocks", 2))
    {

        if (strstr(pszFname, ".map") != NULL ||
            strstr(pszFname, ".MAP") != NULL)
        {
            DumpMapFileBlocks(pszFname);
        }
        else if (strstr(pszFname, ".ind") != NULL ||
                 strstr(pszFname, ".IND") != NULL)
        {
            DumpIndFileObjects(pszFname);
        }
        else if (strstr(pszFname, ".otherextension") != NULL)
        {
            ;
        }
    }
/*---------------------------------------------------------------------
 *      With option -blocks <filename>
 *      Open file, and dump each block sequentially.
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-details", 2) && argc >= 4)
    {

        if (strstr(pszFname, ".map") != NULL ||
            strstr(pszFname, ".MAP") != NULL)
        {
            DumpMapFileBlockDetails(pszFname, atoi(argv[3]));
        }
        else if (strstr(pszFname, ".otherextension") != NULL)
        {
            ;
        }
    }
/*---------------------------------------------------------------------
 *      With option -objects <filename>
 *      Open file, and all geogr. objects.
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-objects", 2))
    {

        if (strstr(pszFname, ".map") != NULL ||
            strstr(pszFname, ".MAP") != NULL)
        {
            DumpMapFileObjects(pszFname);
        }
        else if (strstr(pszFname, ".tab") != NULL ||
                 strstr(pszFname, ".TAB") != NULL)
        {
            DumpTabFile(pszFname);
        }
        else if (strstr(pszFname, ".ind") != NULL ||
                 strstr(pszFname, ".IND") != NULL)
        {
            DumpIndFileObjects(pszFname);
        }
    }
/*---------------------------------------------------------------------
 *      With option -spatialindex <filename> [maxdepth]
 *      Dump Spatial Index Tree of .MAP file
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-spatialindex", 2))
    {

        if (strstr(pszFname, ".map") != NULL ||
            strstr(pszFname, ".MAP") != NULL)
        {
            if (argc >= 4)
                DumpMapFileIndexTree2MIF(pszFname, atoi(argv[3]));
            else
                DumpMapFileIndexTree2MIF(pszFname, -1);
        }
    }
/*---------------------------------------------------------------------
 *      With option -all <filename>
 *      Dump the whole TAB dataset (all supported files)
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-all", 2))
    {

        if (strstr(pszFname, ".tab") != NULL ||
            strstr(pszFname, ".TAB") != NULL ||
            strstr(pszFname, ".mif") != NULL ||
            strstr(pszFname, ".MIF") != NULL)
        {
            DumpTabFile(pszFname);
        }
    }
/*---------------------------------------------------------------------
 *      With option -coordsys <filename>
 *      Dump the dataset's projection string
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-coordsys", 2))
    {

        if (strstr(pszFname, ".tab") != NULL ||
            strstr(pszFname, ".TAB") != NULL)
        {
            DumpCoordsys(pszFname);
        }
    }
/*---------------------------------------------------------------------
 *      With option -Ccoordsys <filename>
 *      Dump the dataset's coordsys and bounds info in a C struct format
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-Ccoordsys", 3))
    {

        if (strstr(pszFname, ".tab") != NULL ||
            strstr(pszFname, ".TAB") != NULL)
        {
            DumpCoordsysStruct(pszFname);
        }
    }
/*---------------------------------------------------------------------
 *     With option -index <filename> <indexno> <value> 
 *     Search specified index for a value.
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-index", 2) && argc >=5)
    {
        SearchIndex(pszFname, atoi(argv[3]), argv[4]);
    }
/*---------------------------------------------------------------------
 *      With option -envelope <filename> <xmin> <ymin> <ymax> <ymax>
 *      Dump all objects that intersect the envelope.  Scan via spatial index.
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-envelope", 2) && argc >= 7)
    {

        if (strstr(pszFname, ".tab") != NULL ||
            strstr(pszFname, ".TAB") != NULL)
        {
            DumpViaSpatialIndex(pszFname, atof(argv[3]), atof(argv[4]), 
                                atof(argv[5]), atof(argv[6]));
        }
    }
/*---------------------------------------------------------------------
 *     With option -otheroption <filename> ... 
 *     ... do something else ...
 *--------------------------------------------------------------------*/
    else if (EQUALN(argv[1], "-otheroption", 2))
    {
        ;
    }
    else
    {
        printf("Cannot process file %s\n\n", pszFname);
        printf("%s", TABTEST_USAGE);
        
        return 1;
    }

    return 0;
}


/**********************************************************************
 *                          DumpMapFileBlocks()
 *
 * Read and dump a .MAP file... simply dump all blocks sequentially.
 **********************************************************************/
static int DumpMapFileBlocks(const char *pszFname)
{
    FILE        *fp;
    TABRawBinBlock *poBlock;
    int         nOffset = 0;
    VSIStatBuf  sStatBuf;

    /*---------------------------------------------------------------------
     * Try to open source file
     * Note: we use stat() to fetch the file size.
     *--------------------------------------------------------------------*/
    if ( VSIStat(pszFname, &sStatBuf) == -1 )
    {
        printf("stat() failed for %s\n", pszFname);
        return -1;
    }

    fp = fopen(pszFname, "rb");
    if (fp == NULL)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }


    /*---------------------------------------------------------------------
     * Read/Dump blocks until EOF is reached
     *--------------------------------------------------------------------*/
    while (nOffset < sStatBuf.st_size )
    {
        poBlock = TABCreateMAPBlockFromFile(fp, nOffset, 512);

        if (poBlock)
        {
            poBlock->Dump();
            printf("\n");
            delete poBlock;
        }
        else
        {
            // An error happened (could be EOF)... abort now.
            break;
        }

        nOffset += 512;
    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    fclose(fp);

    return 0;
}


/**********************************************************************
 *                          DumpMapFileObjects()
 *
 * Open a .MAP file and print all the geogr. objects found.
 **********************************************************************/
static int DumpMapFileObjects(const char *pszFname)
{
    TABMAPFile  oMAPFile;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if (oMAPFile.Open(pszFname, "rb") != 0)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    oMAPFile.Dump();

    /*---------------------------------------------------------------------
     * Read/Dump objects until EOF is reached
     *--------------------------------------------------------------------*/
    while ( 0 )
    {

    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    oMAPFile.Close();

    return 0;
}


/**********************************************************************
 *                          DumpMapFileIndexTree2MIF()
 *
 * Open a .MAP file and dump the index tree to a .MIF file
 **********************************************************************/
static int DumpMapFileIndexTree2MIF(const char *pszFname, int nMaxDepth)
{
    TABMAPFile  oMAPFile;
    FILE *fpMIF, *fpMID;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if (oMAPFile.Open(pszFname, "rb") != 0)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    /*---------------------------------------------------------------------
     * Create MIF/MID dataset
     *--------------------------------------------------------------------*/
    if ((fpMIF=VSIFOpen(CPLSPrintf("%s.spindex.mif",pszFname),"wt"))==NULL)
    {
        printf("Unable to create %s\n", CPLSPrintf("%s.spindex.mif",pszFname));
        return -1;
    }

    if ((fpMID=VSIFOpen(CPLSPrintf("%s.spindex.mid",pszFname),"wt"))==NULL)
    {
        printf("Unable to create %s\n", CPLSPrintf("%s.spindex.mid",pszFname));
        return -1;
    }

    printf("Dumping spatial index from %s to %s.spindex.mif/.mid\n", 
           pszFname, pszFname);

    VSIFPrintf(fpMIF, 
               "VERSION 300\n"
               "CHARSET \"Neutral\"\n"
               "DELIMITER \",\"\n"
               "COLUMNS 9\n"
               "  ID        integer\n"
               "  PARENT_ID integer\n"
               "  ID_IN_NODE integer\n"
               "  DEPTH     integer\n"
               "  AREA      integer\n"
               "  XMIN      integer\n"
               "  YMIN      integer\n"
               "  XMAX      integer\n"
               "  YMAX      integer\n"
               "DATA\n");

    /*---------------------------------------------------------------------
     * Dump spatial Index Tree
     *--------------------------------------------------------------------*/
    oMAPFile.DumpSpatialIndexToMIF(NULL, fpMIF, fpMID, -1, -1, 0, nMaxDepth);

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    oMAPFile.Close();

    VSIFClose(fpMIF);
    VSIFClose(fpMID);

    return 0;
}

/**********************************************************************
 *                          DumpMapFileBlockDetails()
 *
 * Read and dump specified map file block.
 **********************************************************************/
static int DumpMapFileBlockDetails(const char *pszFname, int nOffset)
{
    FILE        *fp;
    TABRawBinBlock *poBlock;

    /*---------------------------------------------------------------------
     * Try to open source file
     * Note: we use stat() to fetch the file size.
     *--------------------------------------------------------------------*/
    fp = fopen(pszFname, "rb");
    if (fp == NULL)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    /*---------------------------------------------------------------------
     * Read/Dump blocks until EOF is reached
     *--------------------------------------------------------------------*/
    poBlock = TABCreateMAPBlockFromFile(fp, nOffset, 512);

    if (poBlock)
    {
        switch(poBlock->GetBlockClass())
        {
          case TABMAP_OBJECT_BLOCK:
            ((TABMAPObjectBlock*)poBlock)->Dump(NULL, TRUE);
            break;
          default:
            poBlock->Dump(NULL);
        }

        printf("\n");
        delete poBlock;
    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    fclose(fp);

    return 0;
}


/**********************************************************************
 *                          DumpTabFile()
 *
 * Open a .TAB file and print all the geogr. objects found.
 **********************************************************************/
static int DumpTabFile(const char *pszFname)
{
    IMapInfoFile  *poFile;
    int      nFeatureId;
    TABFeature *poFeature;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if ((poFile = IMapInfoFile::SmartOpen(pszFname)) == NULL)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    poFile->Dump();

    /*---------------------------------------------------------------------
     * Check for indexed fields
     *--------------------------------------------------------------------*/
    for(int iField=0; iField<poFile->GetLayerDefn()->GetFieldCount(); iField++)
    {
        if (poFile->IsFieldIndexed(iField))
            printf("  Field %d is indexed\n", iField);
    }

    /*---------------------------------------------------------------------
     * Read/Dump objects until EOF is reached
     *--------------------------------------------------------------------*/
    nFeatureId = -1;
    while ( (nFeatureId = poFile->GetNextFeatureId(nFeatureId)) != -1 )
    {
        poFeature = poFile->GetFeatureRef(nFeatureId);
        if (poFeature)
        {
//            poFeature->DumpReadable(stdout);
            printf("\nFeature %d:\n", nFeatureId);
            poFeature->DumpMID();
            poFeature->DumpMIF();
        }
        else
            break;      // GetFeatureRef() failed: Abort the loop
    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    poFile->Close();

    delete poFile;

    return 0;
}


/**********************************************************************
 *                          DumpIndFileObjects()
 *
 * Read and dump a .IND file
 **********************************************************************/
static int DumpIndFileObjects(const char *pszFname)
{
    TABINDFile  oINDFile;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if (oINDFile.Open(pszFname, "rb") != 0)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    // oINDFile.SetIndexFieldType(1,TABFChar);
    oINDFile.Dump();

    /*---------------------------------------------------------------------
     * Read/Dump objects until EOF is reached
     *--------------------------------------------------------------------*/
    while ( 0 )
    {

    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    oINDFile.Close();

    return 0;
}

/**********************************************************************
 *                          DumpCoordsys()
 *
 * Open a .TAB file and dump coordsys info
 **********************************************************************/
static int DumpCoordsys(const char *pszFname)
{
    IMapInfoFile  *poFile;
    double dXMin, dYMin, dXMax, dYMax;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if ((poFile = IMapInfoFile::SmartOpen(pszFname)) == NULL)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    OGRSpatialReference *poSRS = poFile->GetSpatialRef();
    char *pszCoordSys = MITABSpatialRef2CoordSys(poSRS);
    char *pszProjString=NULL;

    printf("CoordSys %s\n", pszCoordSys?pszCoordSys:"(null)");

    if (poFile->GetBounds(dXMin, dYMin, dXMax, dYMax) == 0)
    {
        printf("  Proj. Bounds (%.15g %.15g) (%.15g %.15g)\n", dXMin, dYMin, dXMax, dYMax);
        printf("    dX dY = %.15g %.15g\n", dXMax - dXMin, dYMax - dYMin);

    }
    else
    {
        printf("  Projection Bounds not available!\n");
    }

    OGREnvelope oEnv;
    if (poFile->GetExtent(&oEnv, TRUE) == 0)
    {
        printf("  Data Extents (%.15g %.15g) (%.15g %.15g)\n", oEnv.MinX, oEnv.MinY, oEnv.MaxX, oEnv.MaxY);

    }
    else
    {
        printf("  Data Extents not available!\n");
    }

    if (poSRS)
    {

//        poSRS->exportToWkt(&pszProjString);
//        printf("  WKT SRS = %s\n", pszProjString);
//        CPLFree(pszProjString);

        poSRS->exportToProj4(&pszProjString);
        printf("  PROJ4 SRS = %s\n", pszProjString);

        // Write bounds to a file and launch 'proj' to convert them to LAT/LON
        FILE *fpOut;
        if (pszProjString &&
            (fpOut = fopen("/tmp/tttbounds.txt", "w")))
        {
            fprintf(fpOut, "%.15g %.15g\n", dXMin, dYMin);
            fprintf(fpOut, "%.15g %.15g\n", dXMax, dYMax);
            fclose(fpOut);

            fflush(stdout);

            system(CPLSPrintf("proj -I %s /tmp/tttbounds.txt", pszProjString));
        }

    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    CPLFree(pszProjString);
    poFile->Close();

    delete poFile;

    return 0;
}

/**********************************************************************
 *                          DumpCoordsysStruct()
 *
 * Open a .TAB file and dump coordsys info in a format usable to build
 * C array of MapInfoBoundsInfo[]
 **********************************************************************/
static int DumpCoordsysStruct(const char *pszFname)
{
    IMapInfoFile  *poFile;
    double dXMin, dYMin, dXMax, dYMax;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if ((poFile = IMapInfoFile::SmartOpen(pszFname)) == NULL)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    TABProjInfo sProjInfo;

    if (poFile->GetProjInfo(&sProjInfo) != 0)
    {
        printf("Cannot fetch TABProjInfo from %s\n", pszFname);
        return -1;
    }

    if (sProjInfo.nProjId == 0)
    {
        printf("Nonearth coordsys in %s\n", pszFname);
        return 0;
    }

    if (poFile->GetBounds(dXMin, dYMin, dXMax, dYMax) == 0)
    {
        printf("{{%d, %d, %d, "
               "{%.15g,%.15g,%.15g,%.15g,%.15g,%.15g}, "
               "%d,%.15g,%.15g,%.15g, "
               "{%.15g,%.15g,%.15g,%.15g,%.15g}}, ", 
               sProjInfo.nProjId,
               sProjInfo.nEllipsoidId,
               sProjInfo.nUnitsId,
               sProjInfo.adProjParams[0],
               sProjInfo.adProjParams[1],
               sProjInfo.adProjParams[2],
               sProjInfo.adProjParams[3],
               sProjInfo.adProjParams[4],
               sProjInfo.adProjParams[5],
               sProjInfo.nDatumId,
               sProjInfo.dDatumShiftX,
               sProjInfo.dDatumShiftY,
               sProjInfo.dDatumShiftZ,
               sProjInfo.adDatumParams[0],
               sProjInfo.adDatumParams[1],
               sProjInfo.adDatumParams[2],
               sProjInfo.adDatumParams[3],
               sProjInfo.adDatumParams[4] );
        

        printf(" %.15g, %.15g, %.15g, %.15g},\n", dXMin, dYMin, dXMax, dYMax);

    }
    else
    {
        printf("  Bounds struct cannot be generated!\n");
    }


    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    poFile->Close();

    delete poFile;

    return 0;
}

/**********************************************************************
 *                          SearchIndex()
 *
 * Search a TAB dataset's .IND file for pszVal in index nIndexNo
 **********************************************************************/
static int SearchIndex(const char *pszFname, int nIndexNo, const char *pszVal)
{
    TABFile     oTABFile;
    TABINDFile  *poINDFile;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if (oTABFile.Open(pszFname, "rb") != 0)
    {
        printf("Failed to open %s as a TABFile.\n", pszFname);
        return -1;
    }

    /*---------------------------------------------------------------------
     * Fetch IND file handle
     *--------------------------------------------------------------------*/
    if ((poINDFile = oTABFile.GetINDFileRef()) == NULL)
    {
        printf("Dataset %s has no .IND file\n", pszFname);
        return -1;
    }

    /*---------------------------------------------------------------------
     * Search the index.
     * For now we search only 'char' index types!!!
     *--------------------------------------------------------------------*/
    GByte *pKey;
    int nRecordNo;

    pKey = poINDFile->BuildKey(nIndexNo, pszVal);
    nRecordNo = poINDFile->FindFirst(nIndexNo, pKey);

    if (nRecordNo < 1)
    {
        printf("Value '%s' not found in index #%d\n", pszVal, nIndexNo);
    }
    else
    {
        while(nRecordNo > 0)
        {
            printf("Record %d...\n", nRecordNo);

            nRecordNo = poINDFile->FindNext(nIndexNo, pKey);
        }
    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    oTABFile.Close();

    return 0;

}

/**********************************************************************
 *                          DumpViaSpatialIndex()
 *
 * Open a .TAB file and print all the geogr. objects that match the 
 * specified filter.  Scanes the file via the spatial index.
 **********************************************************************/
static int DumpViaSpatialIndex(const char *pszFname, 
                               double dXMin, double dYMin, 
                               double dXMax, double dYMax)
{
    IMapInfoFile  *poFile;
    TABFeature *poFeature;

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if ((poFile = IMapInfoFile::SmartOpen(pszFname)) == NULL)
    {
        printf("Failed to open %s\n", pszFname);
        return -1;
    }

    poFile->Dump();

    /*---------------------------------------------------------------------
     * Check for indexed fields
     *--------------------------------------------------------------------*/
    for(int iField=0; iField<poFile->GetLayerDefn()->GetFieldCount(); iField++)
    {
        if (poFile->IsFieldIndexed(iField))
            printf("  Field %d is indexed\n", iField);
    }


    /*---------------------------------------------------------------------
     * Set spatial filter
     *--------------------------------------------------------------------*/
    OGRLinearRing oSpatialFilter;

    oSpatialFilter.setNumPoints(5);
    oSpatialFilter.setPoint(0, dXMin, dYMin);
    oSpatialFilter.setPoint(1, dXMax, dYMin);
    oSpatialFilter.setPoint(2, dXMax, dYMax);
    oSpatialFilter.setPoint(3, dXMin, dYMax);
    oSpatialFilter.setPoint(4, dXMin, dYMin);

    poFile->SetSpatialFilter( &oSpatialFilter );

    /*---------------------------------------------------------------------
     * Read/Dump objects until EOF is reached
     *--------------------------------------------------------------------*/
    while ( (poFeature = (TABFeature*)poFile->GetNextFeature()) != NULL )
    {
//        poFeature->DumpReadable(stdout);
        printf("\nFeature %ld:\n", poFeature->GetFID());
        poFeature->DumpMID();
        poFeature->DumpMIF();
    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    poFile->Close();

    delete poFile;

    return 0;
}

