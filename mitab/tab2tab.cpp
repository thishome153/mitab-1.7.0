/**********************************************************************
 * $Id: tab2tab.cpp,v 1.17 2007/06/12 12:50:40 dmorissette Exp $
 *
 * Name:     tab2tab.cpp
 * Project:  MapInfo TAB format Read/Write library
 * Language: C++
 * Purpose:  Copy features from a .TAB dataset to a new one.
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
 * $Log: tab2tab.cpp,v $
 * Revision 1.17  2007/06/12 12:50:40  dmorissette
 * Use Quick Spatial Index by default until bug 1732 is fixed (broken files
 * produced by current coord block splitting technique).
 *
 * Revision 1.16  2007/06/05 13:03:15  dmorissette
 * Fixed a few minor leaks when fatal errors happen in Tab2Tab()
 *
 * Revision 1.15  2007/03/21 21:15:56  dmorissette
 * Added SetQuickSpatialIndexMode() which generates a non-optimal spatial
 * index but results in faster write time (bug 1669)
 *
 * Revision 1.14  2006/11/20 20:05:58  dmorissette
 * First pass at improving generation of spatial index in .map file (bug 1585)
 * New methods for insertion and splittung in the spatial index are done.
 * Also implemented a method to dump the spatial index to .mif/.mid
 * Still need to implement splitting of TABMapObjectBlock to get optimal
 * results.
 *
 * Revision 1.13  2006/06/28 10:21:56  dmorissette
 * Set dataset bounds only after setting spatialref (bug 1511)
 *
 * Revision 1.12  2004/06/30 20:29:04  dmorissette
 * Fixed refs to old address danmo@videotron.ca
 *
 * Revision 1.11  2004/06/04 19:08:35  dmorissette
 * Updated ref to lib's homepage in usage instructions
 *
 * Revision 1.10  2002/04/22 13:49:09  julien
 * Add EOF validation in MIDDATAFile::GetLastLine() (Bug 819)
 *
 * Revision 1.9  2001/03/09 03:53:30  daniel
 * Added missing delete poDstFile
 *
 * Revision 1.8  2001/01/23 21:23:42  daniel
 * Added projection bounds lookup table, called from TABFile::SetProjInfo()
 *
 * Revision 1.7  2000/10/03 21:46:08  daniel
 * Support MIF output as well, based on output filename extension, making
 * tab2mif.cpp obsolete.
 *
 * Revision 1.6  2000/02/28 17:13:48  daniel
 * Support for creating TABViews, and pass complete indexed field information
 *
 * Revision 1.5  2000/01/15 22:30:45  daniel
 * Switch to MIT/X-Consortium OpenSource license
 *
 * Revision 1.4  1999/12/14 02:24:20  daniel
 * *** empty log message ***
 *
 * Revision 1.3  1999/10/06 13:25:25  daniel
 * Pass file bounds
 *
 * Revision 1.2  1999/10/01 03:43:36  daniel
 * Pass ProjInfo
 *
 * Revision 1.1  1999/09/26 14:59:38  daniel
 * Implemented write support
 *
 **********************************************************************/


#include "mitab.h"
#include <ctype.h>

static int Tab2Tab(const char *pszSrcFname, const char *pszDstFname,
                   int nMaxFeatures, 
                   GBool bQuickSpatialIndexMode, GBool bOptSpatialIndexMode);


/**********************************************************************
 *                          main()
 *
 **********************************************************************/
int main(int argc, char *argv[])
{
    const char  *pszSrcFname, *pszDstFname;
    int nMaxFeatures = -1;
    GBool bQuickSpatialIndexMode = FALSE;
    GBool bOptSpatialIndexMode = FALSE;

/*---------------------------------------------------------------------
 *      Read program arguments.
 *--------------------------------------------------------------------*/
    if (argc<3)
    {
        printf("\nTAB2TAB Conversion Program - MITAB Version %s\n\n", MITAB_VERSION);
        printf("Usage: tab2tab <src_filename> <dst_filename> [-q|-o] [-n num_features]\n");
        printf("    Converts TAB or MIF file <src_filename> to TAB or MIF format.\n");
        printf("    The extension of <dst_filename> (.tab or .mif) defines the output format.\n\n");
        printf("For the latest version of this program and of the library, see: \n");
        printf("    http://mitab.maptools.org/\n\n");
        return 1;
    }
    else
    {
        pszSrcFname = argv[1];
        pszDstFname = argv[2];
    }
    
    for(int iArg = 3; iArg < argc; iArg++)
    {
        if (EQUAL(argv[iArg], "-q"))
            bQuickSpatialIndexMode = TRUE;
        if (EQUAL(argv[iArg], "-o"))
            bOptSpatialIndexMode = TRUE;
        else if (EQUAL(argv[iArg], "-n") && iArg+1 < argc)
            nMaxFeatures = atoi(argv[++iArg]);
    }

    return Tab2Tab(pszSrcFname, pszDstFname, 
                   nMaxFeatures, bQuickSpatialIndexMode, bOptSpatialIndexMode);
}


/**********************************************************************
 *                          Tab2Tab()
 *
 * Copy features from source dataset to a new dataset
 **********************************************************************/
static int Tab2Tab(const char *pszSrcFname, const char *pszDstFname,
                   int nMaxFeatures, 
                   GBool bQuickSpatialIndexMode, GBool bOptSpatialIndexMode)
{
    IMapInfoFile *poSrcFile = NULL, *poDstFile = NULL;
    int      nFeatureId, iField, numFeatures=0;
    TABFeature *poFeature;
    double dXMin, dYMin, dXMax, dYMax;

    /*---------------------------------------------------------------------
     * If there is a "micdsys.txt" in current directory then load it
     *--------------------------------------------------------------------*/
    MITABLoadCoordSysTable("micdsys.txt");

    /*---------------------------------------------------------------------
     * Try to open source file
     *--------------------------------------------------------------------*/
    if ((poSrcFile = IMapInfoFile::SmartOpen(pszSrcFname)) == NULL)
    {
        printf("Failed to open %s\n", pszSrcFname);
        return -1;
    }

    OGRFeatureDefn *poDefn = poSrcFile->GetLayerDefn();

    /*---------------------------------------------------------------------
     * The extension of the output filename tells us if we should create
     * a MIF or a TAB file for output.
     *--------------------------------------------------------------------*/
    if (EQUAL(".mif", pszDstFname + strlen(pszDstFname)-4) ||
        EQUAL(".mid", pszDstFname + strlen(pszDstFname)-4) )
    {
        // Create a MIF file
        poDstFile = new MIFFile;
    }
    else
    {
        /*-----------------------------------------------------------------
         * Create a TAB dataset.
         * Find out if the file contains at least 1 unique field... if so we
         * will create a TABView instead of a TABFile
         *----------------------------------------------------------------*/
        GBool    bFoundUniqueField = FALSE;
        for(iField=0; iField< poDefn->GetFieldCount(); iField++)
        {
            if (poSrcFile->IsFieldUnique(iField))
                bFoundUniqueField = TRUE;
        }

        if (bFoundUniqueField)
            poDstFile = new TABView;
        else
            poDstFile = new TABFile;
    }

    /*---------------------------------------------------------------------
     * Try to open destination file
     *--------------------------------------------------------------------*/
    if (poDstFile->Open(pszDstFname, "wb") != 0)
    {
        printf("Failed to open %s\n", pszDstFname);
        poSrcFile->Close();
        delete poSrcFile;
        delete poDstFile;
        return -1;
    }

    if ( (bQuickSpatialIndexMode && 
          poDstFile->SetQuickSpatialIndexMode(TRUE) != 0) ||
         (bOptSpatialIndexMode && 
          poDstFile->SetQuickSpatialIndexMode(FALSE) != 0) )
    {
        printf("Failed setting Quick Spatial Index Mode (-q|-o) on %s\n", pszDstFname);
        poSrcFile->Close();
        delete poSrcFile;
        poDstFile->Close();
        delete poDstFile;
        return -1;
    }

    // Pass Proj. info directly
    // TABProjInfo sProjInfo;
    // if (poSrcFile->GetProjInfo(&sProjInfo) == 0)
    //     poDstFile->SetProjInfo(&sProjInfo);

    OGRSpatialReference *poSR;

    poSR = poSrcFile->GetSpatialRef();
    if( poSR != NULL )
    {
        poDstFile->SetSpatialRef( poSR );
    }

    //  Set bounds (must be done after setting spatialref)
    if (poSrcFile->GetBounds(dXMin, dYMin, dXMax, dYMax) == 0)
        poDstFile->SetBounds(dXMin, dYMin, dXMax, dYMax);

    /*---------------------------------------------------------------------
     * Pass compplete fields information
     *--------------------------------------------------------------------*/
    for(iField=0; iField< poDefn->GetFieldCount(); iField++)
    {
        OGRFieldDefn *poFieldDefn = poDefn->GetFieldDefn(iField);

        poDstFile->AddFieldNative(poFieldDefn->GetNameRef(),
                                  poSrcFile->GetNativeFieldType(iField),
                                  poFieldDefn->GetWidth(),
                                  poFieldDefn->GetPrecision(),
                                  poSrcFile->IsFieldIndexed(iField),
                                  poSrcFile->IsFieldUnique(iField));
    }

    /*---------------------------------------------------------------------
     * Copy objects until EOF is reached
     *--------------------------------------------------------------------*/
    nFeatureId = -1;
    while ( (nFeatureId = poSrcFile->GetNextFeatureId(nFeatureId)) != -1 &&
            (nMaxFeatures < 1 || numFeatures++ < nMaxFeatures ))
    {
        poFeature = poSrcFile->GetFeatureRef(nFeatureId);
        if (poFeature)
        {
//            poFeature->DumpReadable(stdout);
//            poFeature->DumpMIF();
            poDstFile->SetFeature(poFeature);
        }
        else
        {
            printf( "Failed to read feature %d.\n",
                    nFeatureId );
            return -1;      // GetFeatureRef() failed: Error
        }
    }

    /*---------------------------------------------------------------------
     * Cleanup and exit.
     *--------------------------------------------------------------------*/
    poDstFile->Close();
    delete poDstFile;

    poSrcFile->Close();
    delete poSrcFile;

    MITABFreeCoordSysTable();

    return 0;
}


