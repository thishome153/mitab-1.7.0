'/**********************************************************************
' * $Id: mitab_vb_test.vb,v 1.2 2003/09/09 21:22:41 dmorissette Exp $
' *
' * Name:     mitab_vb_test.vb
' * Project:  MapInfo TAB Read/Write library
' * Language: VB
' * Purpose:  Test mainline for MITAB C API in Visual Basic
' * Author:   Bo Thomsen, bvt@geocon.dk
' *
' **********************************************************************
' * Copyright (c) 2002, Bo Thomsen
' *
' * Permission is hereby granted, free of charge, to any person obtaining a
' * copy of this software and associated documentation files (the "Software"),
' * to deal in the Software without restriction, including without limitation
' * the rights to use, copy, modify, merge, publish, distribute, sublicense,
' * and/or sell copies of the Software, and to permit persons to whom the
' * Software is furnished to do so, subject to the following conditions:
' *
' * The above copyright notice and this permission notice shall be included
' * in all copies or substantial portions of the Software.
' *
' * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
' * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
' * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
' * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
' * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
' * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
' * DEALINGS IN THE SOFTWARE.
' **********************************************************************
' *
' * $Log: mitab_vb_test.vb,v $
' * Revision 1.2  2003/09/09 21:22:41  dmorissette
' * Update from BVT to work with version 1.2.4
' *
' * Revision 1.1  2002/09/07 17:58:01  daniel
' * Initial revision (from BVT)
' *
' * Revision 1.0  2002/09/07 14:53:59  bvt
' * translation of mitabc_test.c to Visual Basic
' *
' */

'/************************************************************************/
'/*                              testReportFile                              */
'/************************************************************************/

Sub testReportfile()
  Call ReportFile("c:\temp\testtab.tab", "c:\temp\testtab.txt")
End Sub

'/************************************************************************/
'/*                              ReportFile                              */
'/************************************************************************/

Sub ReportFile(ByVal pszFilename As String, ByVal pszReportname As String)

    Dim dataset As Long, feature_id As Long, num_fields As Long
    Dim feature As Long, feature_type As Long, num_parts As Long, partno As Long, pointno As Long, fieldno As Long
    Dim num_points As Long, dX As Double, dY As Double
    Dim sTmp As String, sTmp2 As String, i1 As Long, i2 As Long
        
    dataset = mitab_c_open(pszFilename)

    If (dataset = 0) Then
      sTmp = Space$(255)
      i1 = mitab_c_getlasterrormsg_vb(sTmp, 255)
      MsgBox ("mitab_c_open: " & pszFilename & " failed." & vbCrLf & Left$(sTmp, i1))
      Exit Sub
    End If

    On Error Resume Next
    Open pszReportname For Output As #1
    If Err.Number <> 0 Then
      MsgBox ("Error opening file " & pszReportname & Chr$(13) & Str(Err.Number) & " was generated by " & Err.Source & Chr(13) & Err.Description)
      Exit Sub
    End If
    On Error GoTo 0

    Print #1, "Filename: "; pszFilename
    sTmp = Space$(255)
    i1 = mitab_c_get_mif_coordsys_vb(dataset, sTmp, 255)
    Print #1, "Coodsys Clause: "; Left$(sTmp, i1)

    num_fields = mitab_c_get_field_count(dataset)

    feature_id = mitab_c_next_feature_id(dataset, -1)
    Do While feature_id <> -1

'/* -------------------------------------------------------------------- */
'/*      Read next feature object                                        */
'/* -------------------------------------------------------------------- */
      feature = mitab_c_read_feature(dataset, feature_id)
      If (feature = 0) Then
        sTmp = Space$(255)
        i1 = mitab_c_getlasterrormsg_vb(sTmp, 255)
        MsgBox ("Failed to read feature " & Str$(feature_id) & vbCrLf & Left$(sTmp, i1))
        Exit Sub
      End If

      feature_type = mitab_c_get_type(feature)
      num_parts = mitab_c_get_parts(feature)

      Print #1, ""
      Print #1, "Read feature " & Str$(feature_id) & ": type=" & Str$(feature_type) & ", num_parts=" & Str$(num_parts)

'/* -------------------------------------------------------------------- */
'/*      Dump the feature attributes...                                  */
'/* -------------------------------------------------------------------- */
      For fieldno = 0 To num_fields - 1
        sTmp = Space$(255)
        i1 = mitab_c_get_field_name_vb(dataset, fieldno, sTmp, 255)
        sTmp2 = Space$(255)
        i2 = mitab_c_get_field_as_string_vb(feature, fieldno, sTmp2, 255)
        Print #1, "  " & Left$(sTmp, i1) & "=" & Left$(sTmp2, i2)
      Next

'/* -------------------------------------------------------------------- */
'/*      ... and coordinates.                                            */
'/*      In real applications, we would probably want to handle each     */
'/*      object type differently but we won't do it here.                */
'/* -------------------------------------------------------------------- */
      For partno = 0 To num_parts - 1

        Print #1, "  Part no " & Str$(partno + 1)

        num_points = mitab_c_get_vertex_count(feature, partno)
        For pointno = 0 To num_points - 1
          dX = mitab_c_get_vertex_x(feature, partno, pointno)
          dY = mitab_c_get_vertex_y(feature, partno, pointno)
          Print #1, "    "; dX; dY
        Next
      Next

      mitab_c_destroy_feature (feature)
      feature_id = mitab_c_next_feature_id(dataset, feature_id)
    Loop

    mitab_c_close (dataset)

    On Error Resume Next
    Close #1
    On Error GoTo 0
    
    If (mitab_c_getlasterrorno() <> 0) Then
      sTmp = Space$(255)
      i1 = mitab_c_getlasterrormsg_vb(sTmp, 255)
      MsgBox "Last Error: " & Left$(sTmp, i1)
    End If

End Sub

'/************************************************************************/
'/*                              CopyFile()                              */
'/************************************************************************/

Sub CopyFile(ByVal pszSource As String, ByVal pszDest As String)
  MsgBox ("Copy File not implemented at this time.")
End Sub

'/************************************************************************/
'/*                              testWriteFile                           */
'/************************************************************************/

Sub testWritefile()
  Call WriteFile("c:\temp\testtab.tab", "tab")
End Sub

'/************************************************************************/
'/*                             WriteFile()                              */
'/************************************************************************/

Sub WriteFile(ByVal pszDest As String, ByVal pszMifOrTab As String)

    Dim dataset As Long, feature As Long, x(0 To 99) As Double, y(0 To 99) As Double, field_index As Long
    Dim sTmp As String, i1 As Long, i2 As Long
    
    dataset = mitab_c_create(pszDest, pszMifOrTab, "CoordSys Earth Projection 1, 0", 90, 0, 180, -180)

    If (dataset = 0) Then
      sTmp = Space$(255)
      i1 = mitab_c_getlasterrormsg_vb(sTmp, 255)
      MsgBox ("Failed to create " & pszMifOrTab & " file: " & pszDest & vbCrLf & Left$(sTmp, i1))
      Exit Sub
    End If

'/* -------------------------------------------------------------------- */
'/*      Add a text, float and integer field.                            */
'/* -------------------------------------------------------------------- */
    field_index = mitab_c_add_field(dataset, "TestInt", TABFT_Integer, 8, 0, 0, 0)
    field_index = mitab_c_add_field(dataset, "TestFloat", TABFT_Float, 12, 2, 0, 0)
    field_index = mitab_c_add_field(dataset, "TestString", TABFT_Char, 10, 0, 0, 0)

'/* -------------------------------------------------------------------- */
'/*      Write a point.                                                  */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Point)

    x(0) = 98
    y(0) = 50
    
    Call mitab_c_set_points(feature, 0, 1, x(0), y(0))
    i1 = 256
    i2 = 255
    i1 = i1 * i2
    Call mitab_c_set_symbol(feature, 41, 15, i1)
    Call mitab_c_set_field(feature, 0, "100")
    Call mitab_c_set_field(feature, 1, "100.5")
    Call mitab_c_set_field(feature, 2, "12345678901234567890")
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)

'/* -------------------------------------------------------------------- */
'/*      Write a MultiPoint.                                                  */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_MultiPoint)

    x(0) = 90
    y(0) = 51
    x(1) = 90.5
    y(1) = 51.5
    x(2) = 91
    y(2) = 52
    
    Call mitab_c_set_points(feature, 0, 3, x(0), y(0))
    i1 = 256
    i2 = 255
    i1 = i1 * i2
    Call mitab_c_set_symbol(feature, 41, 15, i1)
    Call mitab_c_set_field(feature, 0, "100")
    Call mitab_c_set_field(feature, 1, "100.5")
    Call mitab_c_set_field(feature, 2, "12345678901234567890")
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)

'/* -------------------------------------------------------------------- */
'/*      Write a line.                                                   */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Polyline)

    x(0) = 100
    y(0) = 49
    x(1) = 101
    y(1) = 48
    
    Call mitab_c_set_points(feature, 0, 2, x(0), y(0))
    Call mitab_c_set_pen(feature, 1, 2, 255)
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)

'/* -------------------------------------------------------------------- */
'/*      Write text.                                                     */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Text)

    x(0) = 101
    y(0) = 51
    
    Call mitab_c_set_points(feature, 0, 1, x(0), y(0))
    Call mitab_c_set_text(feature, "My text")
    Call mitab_c_set_font(feature, "Arial")
    Call mitab_c_set_text_display(feature, 45#, 1#, 7#, 255 * 65536, 0, -1, -1, -1)
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)
    
'/* -------------------------------------------------------------------- */
'/*      Write region (polygon).                                         */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Region)

    x(0) = 101
    y(0) = 51
    x(1) = 100
    y(1) = 51
    x(2) = 100
    y(2) = 50
    x(3) = 101
    y(3) = 50
    x(4) = 101
    y(4) = 51
    
    Call mitab_c_set_points(feature, 0, 5, x(0), y(0))
    
    x(0) = 100.5
    y(0) = 50.5
    x(1) = 100.5
    y(1) = 50.7
    x(2) = 100.7
    y(2) = 50.7
    x(3) = 100.7
    y(3) = 50.5
    x(4) = 100.5
    y(4) = 50.5

    Call mitab_c_set_points(feature, 1, 5, x(0), y(0))

    Call mitab_c_set_brush(feature, 255, 0, 2, 0)
    Call mitab_c_set_pen(feature, 1, 2, 65535)
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)
    
'/* -------------------------------------------------------------------- */
'/*      Write a second region with two polygon (polygon).               */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Region)

    x(0) = 101
    y(0) = 41
    x(1) = 100
    y(1) = 41
    x(2) = 100
    y(2) = 40
    x(3) = 101
    y(3) = 40
    x(4) = 101
    y(4) = 41
    
    Call mitab_c_set_points(feature, 0, 5, x(0), y(0))
    
    x(0) = 100.5
    y(0) = 40.5
    x(1) = 100.5
    y(1) = 40.7
    x(2) = 100.7
    y(2) = 40.7
    x(3) = 100.7
    y(3) = 40.5
    x(4) = 100.5
    y(4) = 40.5

    Call mitab_c_set_points(feature, 1, 5, x(0), y(0))
    
    x(0) = 100.2
    y(0) = 40.2
    x(1) = 100.2
    y(1) = 40.4
    x(2) = 100.4
    y(2) = 40.4
    x(3) = 100.4
    y(3) = 40.2
    x(4) = 100.2
    y(4) = 40.2

    Call mitab_c_set_points(feature, 2, 5, x(0), y(0))

    x(0) = 96
    y(0) = 46
    x(1) = 95
    y(1) = 46
    x(2) = 95
    y(2) = 45
    x(3) = 96
    y(3) = 45
    x(4) = 96
    y(4) = 46
    
    Call mitab_c_set_points(feature, 0, 5, x(0), y(0))
    
    x(0) = 95.5
    y(0) = 45.5
    x(1) = 95.5
    y(1) = 45.7
    x(2) = 95.7
    y(2) = 45.7
    x(3) = 95.7
    y(3) = 45.5
    x(4) = 95.5
    y(4) = 45.5

    Call mitab_c_set_points(feature, 4, 5, x(0), y(0))

    Call mitab_c_set_brush(feature, 255, 0, 2, 0)
    Call mitab_c_set_pen(feature, 1, 2, 65535)
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)
    
'/* -------------------------------------------------------------------- */
'/*      Write multiple polyline (3 parts).                              */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Polyline)

    x(0) = 111
    y(0) = 57
    x(1) = 110
    y(1) = 57
    x(2) = 110
    y(2) = 56

    Call mitab_c_set_points(feature, 0, 3, x(0), y(0))

    x(0) = 108
    y(0) = 56
    x(1) = 109
    y(1) = 57
    
    Call mitab_c_set_points(feature, 1, 2, x(0), y(0))
    
    x(0) = 105
    y(0) = 55
    x(1) = 105
    y(1) = 57
    x(2) = 107
    y(2) = 57
    x(3) = 107
    y(3) = 55
    
    Call mitab_c_set_points(feature, 2, 4, x(0), y(0))
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)

'/* -------------------------------------------------------------------- */
'/*      Write an arc                                                    */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Arc)

    Call mitab_c_set_arc(feature, 70, 75, 10, 5, 45, 270)
    Call mitab_c_set_field(feature, 0, "123")
    Call mitab_c_set_field(feature, 1, "456")
    Call mitab_c_set_field(feature, 2, "12345678901234567890")
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)

'/* -------------------------------------------------------------------- */
'/*      Write an ellipse                                                */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Ellipse)

    Call mitab_c_set_arc(feature, 70, 75, 10, 5, 0, 0)
    Call mitab_c_set_field(feature, 0, "1")
    Call mitab_c_set_field(feature, 1, "2")
    Call mitab_c_set_field(feature, 2, "3")
    Call mitab_c_set_brush(feature, 255, 0, 2, 0)
    Call mitab_c_set_pen(feature, 1, 2, 65535)
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)

'/* -------------------------------------------------------------------- */
'/*      Write rectangle.                                                */
'/*      The MBR of the array of points will be used for the             */
'/*      rectangle corners.                                              */
'/* -------------------------------------------------------------------- */
    feature = mitab_c_create_feature(dataset, TABFC_Rectangle)

    x(0) = 91
    y(0) = 61
    x(1) = 90
    y(1) = 61
    x(2) = 90
    y(2) = 60
    x(3) = 91
    y(3) = 60
    x(4) = 91
    y(4) = 61
    
    Call mitab_c_set_points(feature, 0, 5, x(0), y(0))
    
    Call mitab_c_set_brush(feature, 255, 0, 2, 0)
    Call mitab_c_set_pen(feature, 1, 2, 65535)
    Call mitab_c_write_feature(dataset, feature)
    Call mitab_c_destroy_feature(feature)

'/* -------------------------------------------------------------------- */
'/*      Cleanup                                                         */
'/* -------------------------------------------------------------------- */
    mitab_c_close (dataset)
    
    If (mitab_c_getlasterrorno() <> 0) Then
      sTmp = Space$(255)
      i1 = mitab_c_getlasterrormsg_vb(sTmp, 255)
      MsgBox "Last Error: " & Left$(sTmp, i1)
    End If

End Sub
