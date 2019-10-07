/**********************************************************************
 * $Id: mitabc_test.c,v 1.15 2005/10/07 18:49:40 dmorissette Exp $
 *
 * Name:     mitabc_test.c
 * Project:  MapInfo TAB Read/Write library
 * Language: C
 * Purpose:  Test mainline for MITAB C API.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 **********************************************************************
 * Copyright (c) 2000-2005, Frank Warmerdam
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
 * $Log: mitabc_test.c,v $
 * Revision 1.15  2005/10/07 18:49:40  dmorissette
 * Added methods for collections in C API (bug 1126)
 *
 * Revision 1.14  2005/04/28 16:18:28  dmorissette
 * Change one of the tests to set a black pen
 *
 * Revision 1.13  2005/04/07 15:56:27  dmorissette
 * Added mitab_c_set_symbol_angle() and mitab_c_get_symbol_angle() for
 * point symbols of type TABFC_FontPoint (bug 1002)
 *
 * Revision 1.12  2003/01/18 21:44:33  daniel
 * Added 'indexed' and 'unique' parameters to mitab_c_add_field().
 *
 * Revision 1.11  2003/01/18 20:43:31  daniel
 * Added support for writing NONE geometries via the C API
 *
 * Revision 1.10  2002/05/16 14:13:44  julien
 * Add test for MultiPoint and Region with MultiPolygon
 *
 * Revision 1.9  2001/12/17 16:08:22  warmerda
 * added a bit of error reporting
 *
 * Revision 1.8  2000/10/03 20:43:36  daniel
 * Added support for writing arcs,ellipses and rectangles in C API
 *
 * Revision 1.7  2000/04/21 12:53:41  daniel
 * Added funcs to fetch feature coordinates and attributes
 *
 * Revision 1.6  2000/02/28 17:20:34  daniel
 * Removed style param to mitab_c_set_pen() since this param is actually
 * used inside the format to define pen width in points (version 450)
 *
 * Revision 1.5  2000/01/15 22:37:02  daniel
 * Switch to MIT/X-Consortium OpenSource license
 *
 * Revision 1.4  2000/01/14 21:58:40  warmerda
 * added error handling
 *
 * Revision 1.3  2000/01/14 17:12:20  warmerda
 * Added some include files.
 *
 * Revision 1.2  2000/01/14 16:33:24  warmerda
 * initial implementation complete
 *
 * Revision 1.1  2000/01/14 14:53:59  warmerda
 * New
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "mitab_capi.h"

#ifndef NULL
#  define NULL 0
#endif

/************************************************************************/
/*                              ReportFile                              */
/************************************************************************/

static void ReportFile( const char * pszFilename )

{
    mitab_handle	dataset;
    int			feature_id, num_fields;
    
    dataset = mitab_c_open( pszFilename );

    if( dataset == NULL )
    {
        printf( "mitab_c_open(%s) failed.\n%s\n",
                pszFilename, mitab_c_getlasterrormsg() );
        exit( 1 );
    }

    num_fields = mitab_c_get_field_count(dataset);

    for( feature_id = mitab_c_next_feature_id(dataset,-1);
         feature_id != -1;
         feature_id = mitab_c_next_feature_id(dataset,feature_id) )
    {
        mitab_feature	feature;
        int feature_type, num_parts, partno, pointno, fieldno;

/* -------------------------------------------------------------------- */
/*      Read next feature object                                        */
/* -------------------------------------------------------------------- */
        feature = mitab_c_read_feature( dataset, feature_id );
        if( feature == NULL )
        {
            printf( "Failed to read feature %d.\n%s\n",
                    feature_id, mitab_c_getlasterrormsg() );
            exit( 1 );
        }

        feature_type = mitab_c_get_type(feature);
        num_parts = mitab_c_get_parts(feature);

        printf( "Read feature %d: type=%d, num_parts=%d.\n", 
                feature_id, feature_type, num_parts  );

/* -------------------------------------------------------------------- */
/*      Dump the feature attributes...                                  */
/* -------------------------------------------------------------------- */
        for(fieldno = 0; fieldno < num_fields; fieldno++)
        {
            printf("  %s = %s\n", 
                     mitab_c_get_field_name(dataset, fieldno),
                     mitab_c_get_field_as_string(feature, fieldno) );
        }

/* -------------------------------------------------------------------- */
/*      ... and coordinates.                                            */
/*      In real applications, we would probably want to handle each     */
/*      object type differently but we won't do it here.                */
/* -------------------------------------------------------------------- */
        for(partno = 0; partno < num_parts; partno++)
        {
            int num_points = mitab_c_get_vertex_count(feature, partno);

            if (num_parts > 1)
                printf(" Part no %d:\n", partno);

            for(pointno = 0; pointno < num_points; pointno++)
            {
                double dX, dY;
                dX = mitab_c_get_vertex_x(feature, partno, pointno);
                dY = mitab_c_get_vertex_y(feature, partno, pointno);

                printf("  %.16g %.16g\n", dX, dY);
            }
        }
        mitab_c_destroy_feature( feature );
    }

    mitab_c_close( dataset );

    if( mitab_c_getlasterrormsg() != NULL 
        && strlen(mitab_c_getlasterrormsg()) > 0 )
        fprintf( stderr, "Last Error: %s\n", mitab_c_getlasterrormsg() );
}

/************************************************************************/
/*                              CopyFile()                              */
/************************************************************************/

static void CopyFile( const char * pszSource, const char * pszDest )

{
    fprintf( stderr, "Copy File not implemented at this time.\n" );
}

/************************************************************************/
/*                             WriteFile()                              */
/************************************************************************/

static void WriteFile( const char * pszDest, const char * pszMifOrTab )

{
    mitab_handle	dataset;
    mitab_feature	feature;
    mitab_feature	region, polyline, multipoint;
    double		x[100], y[100];
    int			field_index;
    
    dataset = mitab_c_create( pszDest, pszMifOrTab,
                              "CoordSys Earth Projection 1, 0",
                              90, 0, 180, -180 );

    if( dataset == NULL )
    {
        printf( "Failed to create %s file %s.\n%s\n",
                pszMifOrTab, pszDest, mitab_c_getlasterrormsg() );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Add a text, float and integer field.                            */
/* -------------------------------------------------------------------- */
    field_index = mitab_c_add_field( dataset, "TestInt",
                                     TABFT_Integer, 8, 0, 
                                     1, 0 ); /* Indexed */
    assert( field_index == 0 );

    field_index = mitab_c_add_field( dataset, "TestFloat", TABFT_Float,
                                     12, 2, 0, 0 );
    assert( field_index == 1 );

    field_index = mitab_c_add_field( dataset, "TestString", TABFT_Char,
                                     10, 0, 0, 0 );
    assert( field_index == 2 );

/* -------------------------------------------------------------------- */
/*      Write a NONE feature (attributes only with no geometry)         */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_NoGeom );

    mitab_c_set_field( feature, 0, "42" );
    mitab_c_set_field( feature, 1, "123.45" );
    mitab_c_set_field( feature, 2, "" );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );

/* -------------------------------------------------------------------- */
/*      Write a point.                                                  */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Point );

    x[0] = 98;
    y[0] = 50;
    
    mitab_c_set_points( feature, 0, 1, x, y );
    mitab_c_set_symbol( feature, 41, 15, 255*256 );
    mitab_c_set_field( feature, 0, "100" );
    mitab_c_set_field( feature, 1, "100.5" );
    mitab_c_set_field( feature, 2, "12345678901234567890" );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );

/* -------------------------------------------------------------------- */
/*      Write a MultiPoint.                                                  */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_MultiPoint );

    x[0] = 90;
    y[0] = 51;
    x[1] = 90.5;
    y[1] = 51.5;
    x[2] = 91;
    y[2] = 52;
    
    mitab_c_set_points( feature, 0, 3, x, y );
    mitab_c_set_symbol( feature, 41, 15, 255*256 );
    mitab_c_set_field( feature, 0, "100" );
    mitab_c_set_field( feature, 1, "100.5" );
    mitab_c_set_field( feature, 2, "12345678901234567890" );
    mitab_c_write_feature( dataset, feature );
    /* Do not destroy it yet, we'll reuse it for the collection example */
    multipoint = feature;

/* -------------------------------------------------------------------- */
/*      Write a font point.                                             */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_FontPoint );

    x[0] = 89;
    y[0] = 65;
    
    mitab_c_set_points( feature, 0, 1, x, y );
    mitab_c_set_symbol( feature, 75, 15, 255*256 );
    mitab_c_set_font( feature, "MySymbolFont" );
    mitab_c_set_symbol_angle( feature, 45 );
    mitab_c_set_field( feature, 0, "100" );
    mitab_c_set_field( feature, 1, "100.5" );
    mitab_c_set_field( feature, 2, "12345678901234567890" );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );

/* -------------------------------------------------------------------- */
/*      Write a line.                                                   */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Polyline );

    x[0] = 100;
    y[0] = 49;
    x[1] = 101;
    y[1] = 48;
    
    mitab_c_set_points( feature, 0, 2, x, y );
    mitab_c_set_pen( feature, 1, 2, 255 );
    mitab_c_write_feature( dataset, feature );

    /* Do not destroy it yet, we'll reuse it for the collection example */
    polyline = feature;

/* -------------------------------------------------------------------- */
/*      Write text.                                                     */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Text );

    x[0] = 101;
    y[0] = 51;
    
    mitab_c_set_points( feature, 0, 1, x, y );
    mitab_c_set_text( feature, "My text" );
    mitab_c_set_font( feature, "Arial" );
    mitab_c_set_text_display( feature, 
                              45.0,     /* angle */
                              1.0, 7.0, /* Text MBR height and width */
                              255*65536,/* FG Color */
                              0,        /* BG Color */
                              -1, -1, -1 );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );
    
/* -------------------------------------------------------------------- */
/*      Write region (polygon).                                         */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Region );

    x[0] = 101;
    y[0] = 51;
    x[1] = 100;
    y[1] = 51;
    x[2] = 100;
    y[2] = 50;
    x[3] = 101;
    y[3] = 50;
    x[4] = 101;
    y[4] = 51;
    
    mitab_c_set_points( feature, 0, 5, x, y );
    
    x[0] = 100.5;
    y[0] = 50.5;
    x[1] = 100.5;
    y[1] = 50.7;
    x[2] = 100.7;
    y[2] = 50.7;
    x[3] = 100.7;
    y[3] = 50.5;
    x[4] = 100.5;
    y[4] = 50.5;

    mitab_c_set_points( feature, 1, 5, x, y );

    mitab_c_set_brush( feature, 255, 0, 2, 0 );
    mitab_c_set_pen( feature, 1, 2, 0 );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );
    
/* -------------------------------------------------------------------- */
/*      Write a second region with two polygon (polygon).               */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Region );

    x[0] = 101;
    y[0] = 41;
    x[1] = 100;
    y[1] = 41;
    x[2] = 100;
    y[2] = 40;
    x[3] = 101;
    y[3] = 40;
    x[4] = 101;
    y[4] = 41;
    
    mitab_c_set_points( feature, 0, 5, x, y );
    
    x[0] = 100.5;
    y[0] = 40.5;
    x[1] = 100.5;
    y[1] = 40.7;
    x[2] = 100.7;
    y[2] = 40.7;
    x[3] = 100.7;
    y[3] = 40.5;
    x[4] = 100.5;
    y[4] = 40.5;

    mitab_c_set_points( feature, 1, 5, x, y );
    
    x[0] = 100.2;
    y[0] = 40.2;
    x[1] = 100.2;
    y[1] = 40.4;
    x[2] = 100.4;
    y[2] = 40.4;
    x[3] = 100.4;
    y[3] = 40.2;
    x[4] = 100.2;
    y[4] = 40.2;

    mitab_c_set_points( feature, 2, 5, x, y );

    x[0] = 96;
    y[0] = 46;
    x[1] = 95;
    y[1] = 46;
    x[2] = 95;
    y[2] = 45;
    x[3] = 96;
    y[3] = 45;
    x[4] = 96;
    y[4] = 46;
    
    mitab_c_set_points( feature, 0, 5, x, y );
    
    x[0] = 95.5;
    y[0] = 45.5;
    x[1] = 95.5;
    y[1] = 45.7;
    x[2] = 95.7;
    y[2] = 45.7;
    x[3] = 95.7;
    y[3] = 45.5;
    x[4] = 95.5;
    y[4] = 45.5;

    mitab_c_set_points( feature, 4, 5, x, y );

    mitab_c_set_brush( feature, 255, 0, 2, 0 );
    mitab_c_set_pen( feature, 1, 2, 65535 );
    mitab_c_write_feature( dataset, feature );

    /* Do not destroy it yet, we'll reuse it for the collection example */
    region = feature;

/* -------------------------------------------------------------------- */
/*      Write multiple polyline (3 parts).                              */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Polyline );

    x[0] = 111;
    y[0] = 57;
    x[1] = 110;
    y[1] = 57;
    x[2] = 110;
    y[2] = 56;

    mitab_c_set_points( feature, 0, 3, x, y );

    x[0] = 108;
    y[0] = 56;
    x[1] = 109;
    y[1] = 57;
    
    mitab_c_set_points( feature, 1, 2, x, y );
    
    x[0] = 105;
    y[0] = 55;
    x[1] = 105;
    y[1] = 57;
    x[2] = 107;
    y[2] = 57;
    x[3] = 107;
    y[3] = 55;
    
    mitab_c_set_points( feature, 2, 4, x, y );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );

/* -------------------------------------------------------------------- */
/*      Write an arc                                                    */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Arc );

    mitab_c_set_arc( feature, 70, 75, 10, 5, 45, 270);
    mitab_c_set_field( feature, 0, "123" );
    mitab_c_set_field( feature, 1, "456" );
    mitab_c_set_field( feature, 2, "12345678901234567890" );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );

/* -------------------------------------------------------------------- */
/*      Write an ellipse                                                */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Ellipse );

    mitab_c_set_arc( feature, 70, 75, 10, 5, 0, 0);
    mitab_c_set_field( feature, 0, "1" );
    mitab_c_set_field( feature, 1, "2" );
    mitab_c_set_field( feature, 2, "3" );
    mitab_c_set_brush( feature, 255, 0, 2, 0 );
    mitab_c_set_pen( feature, 1, 2, 65535 );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );

/* -------------------------------------------------------------------- */
/*      Write rectangle.                                                */
/*      The MBR of the array of points will be used for the             */
/*      rectangle corners.                                              */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Rectangle );

    x[0] = 91;
    y[0] = 61;
    x[1] = 90;
    y[1] = 61;
    x[2] = 90;
    y[2] = 60;
    x[3] = 91;
    y[3] = 60;
    x[4] = 91;
    y[4] = 61;
    
    mitab_c_set_points( feature, 0, 5, x, y );
    
    mitab_c_set_brush( feature, 255, 0, 2, 0 );
    mitab_c_set_pen( feature, 1, 2, 65535 );
    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );



/* -------------------------------------------------------------------- */
/*      Write a collection                                              */
/*      reusing the region, polyline and multipoint objects created     */
/*      above.                                                          */
/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature( dataset, TABFC_Collection );

    mitab_c_set_field( feature, 0, "1" );
    mitab_c_set_field( feature, 1, "2" );
    mitab_c_set_field( feature, 2, "Collection" );

    /* Set the region and polyline parts. 
     * Using make_copy=FALSE means that the region/polyline objects will 
     * be owned by the collection so we don't need to destroy them
     */
    mitab_c_set_collection_region( feature, region, 0);
    mitab_c_set_collection_polyline( feature, polyline, 0 );

    /* Set the multipoint part. 
     * This time using make_copy=TRUE which means that we remain owner of the
     * multipoint and need to destroy it ourselves.
     */
    mitab_c_set_collection_multipoint( feature, multipoint, 1 );
    mitab_c_destroy_feature( multipoint );

    mitab_c_write_feature( dataset, feature );
    mitab_c_destroy_feature( feature );


/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    mitab_c_close( dataset );
    
    if( mitab_c_getlasterrormsg() != NULL 
        && strlen(mitab_c_getlasterrormsg()) > 0 )
        fprintf( stderr, "Last Error: %s\n", mitab_c_getlasterrormsg() );
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc, char ** papszArgv )

{
    if( nArgc < 2 )
    {
        printf( "Usage: mitabc_test src_filename [dst_filename]\n" );
        printf( "    or mitabc_test -w[mif/tab] dst_filename\n" );
        exit( 1 );
    }

    if( nArgc == 2 )
        ReportFile( papszArgv[1] );
    else if( strcmp(papszArgv[1],"-wtab") == 0 )
        WriteFile( papszArgv[2], "tab" );
    else if( strcmp(papszArgv[1],"-wmif") == 0 )
        WriteFile( papszArgv[2], "mif" );
    else
        CopyFile( papszArgv[1], papszArgv[2] );

    exit( 0 );
}







