/*
 * Converted from Turbo Pascal 17-Mar-1997
 *     Emory R. Stagmer
 *         emory@Untiedmusic.com
 *
 *  NOTE: the pascal program generated one .CAM file per frame which
 *  had an entire camera definition.  NOW we generate one include file
 *  (this was originally written for use under POV 2.2 and we didn't
 *   have a 'switch'!).
 *
 *Output:
 *  Will generate sequence of X,Y,Z locations within a switch.
 *  Switch Variable is : HERMITE_FRAME range 0..#frames-1
 *  Output #declare is:  HERMITE_VEC
 *  Output Example:
 *
 *  #switch( HERMITE_FRAME )
 *      #case (0):
 *          #declare HERMITE_VEC=<1,3.432,0>;
 *          #break;
 *      #case (1):
 *          #declare HERMITE_VEC=<1.1,3.523,0>;
 *          #break;
 *      // one case per frame...
 *      #else
 *          #error "Invalid HERMITE_FRAME"
 *          #break;
 *  #end // switch ( HERMITE_FRAME )
 *
 *  The variable HERMITE_FRAME can, of course, be anything, though
 *  typically related to the clock variable, like anything as simple as:
 *
 *      #declare HERMITE_FRAME = clock
 *
 *  If your animation goes from 0 to 1 (a typical scenario), you will need
 *  some kind of 'total frames' variable which can then be multiplied
 *  by the clock to generate a frame number ( or some similar scheme... ).
 *
 *  Revision History:
 *
 *  Date         Remarks
 *  --------     -------------------------------------------------------------
 *  12-May-1997  Changed Output from HERMITE_X,HERMITE_Y,HERMITE_Z to HERMITE_VEC
 *               I was almost always using the whole vector anyway, so this is
 *               MUCH simpler.  If you want any seperate component, use
 *               HERMITE_VEC.X instead of HERMITE_X
 *  12-Nov-1998  (same day of the month??!)  Now putting a ; after each
 *              #declare to get rid of warnings in POV 3.1+ (POVHerm 1.2)
 *
 *  10-Mar-2022 Converted to Visual C++ to allow for compiling for 64 bit systems!
 *              24 years later...damn am I a packrat or what?
 */

#include "math.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#ifndef TRUE
#   define TRUE 1
#   define FALSE 0
#endif

const int  MaxPoints = 1000;
const int  MaxCtrl = 100;

// This is the input record.

class EndPointRecTyp  // The resultant intermediate computed points
    {
    public:
    double x, y, z;
    EndPointRecTyp(double vx=0.0, double vy=0.0, double vz=0.0)
            :x(vx),y(vy),z(vz) {};
    };

class CtrlRecTyp    // The control points of the curves
    {
    public:
    EndPointRecTyp Vpt, DVpt;
    int Frames;
    CtrlRecTyp():Vpt(),DVpt(),Frames(0) {};
    };

static FILE *InFile, *OutFile, *DebugFile;

char InputFileName[255],
    OutputFileName[255] ,
    tmps[255];

EndPointRecTyp VPoints[MaxPoints],
    Look_Ats[MaxPoints];// Arrays for resultant points
CtrlRecTyp CPoints[MaxCtrl];
int CtrlCnt, PointCnt, i, j;

void Read_Input()
    {
    register int i = 0;
    CtrlRecTyp tmp;

    CtrlCnt = 0;
    while (!feof(InFile))
        {
        fscanf_s( InFile, "%lf %lf %lf %lf %lf %lf %d\n",
                &tmp.Vpt.x, &tmp.Vpt.y, &tmp.Vpt.z,
                &tmp.DVpt.x, &tmp.DVpt.y, &tmp.DVpt.z, &tmp.Frames );
        CPoints[i] = tmp;
        i++;
        }
    CtrlCnt = i;
    }


void Start_Output()
    {
    register int i = 0;
    fprintf(OutFile, "// POVHerm 1.2 - now using HERMITE_VEC instead of HERMITE_X,Y,Z (adding semicolons!)\n");
    fprintf(OutFile, "// Echoing %d Control Points\n",CtrlCnt);
    for (i=0; i<CtrlCnt; i++)
        {
        fprintf(OutFile, "// %f %f %f   %f %f %f   %d\n",
            CPoints[i].Vpt.x,CPoints[i].Vpt.y,CPoints[i].Vpt.z,
            CPoints[i].DVpt.x,CPoints[i].DVpt.x,CPoints[i].DVpt.x,
            CPoints[i].Frames
            );
        }
    fprintf(OutFile, "\n#switch(HERMITE_FRAME)\n");
    }

void End_Output()
    {
    fprintf(OutFile, "    #else\n");
    fprintf(OutFile, "        #error \"Invalid Hermite Frame\"\n");
    fprintf(OutFile, "#end // switch HERMITE_FRAME\n");
    }

void Write_Output(EndPointRecTyp &Point, int frame)
    {
    fprintf(OutFile,"    #case (%d)\n", frame);
    fprintf(OutFile,"        #declare HERMITE_VEC = <%f,%f,%f>;\n",
            Point.x, Point.y, Point.z);
    fprintf(OutFile,"        #break;\n");
    }


void Hermite3D(void)
    {
    register int Current, Next, divisor;

    // C1..C4 are from Foley/VanDam eq (13.30) p.517 - the "t" equations.}
    double t, t2, t3, Delta, C1, C2, C3, C4;
    int Done, FirstTime, Completed; // booleans

    // Vxx variables are for view origin
    EndPointRecTyp VP1, VP2, VtempPnt1, VtempPnt2, VtempPoint;

    Current = 0;
    Next = 1;
    FirstTime = TRUE;
    Completed = FALSE;
    PointCnt = 0;

    while(!Completed)
        {
#ifdef DEBUG
fprintf(DebugFile,"In Main Loop in Hermite...\n");
#endif

        //
        //    First time through we want to start at the t=0 point.
        //    However if we did that each time, we"d end up with a duplicate
        //    point (where t=1 and t=0) for each end-point, so start at the
        //    Delta for each control point after the first.
        //    Which means we also need to change the number of frames for the
        //    last point so we end up where we think we will...
        //

        divisor = CPoints[Current].Frames;
        if (Next == CtrlCnt-1)
            divisor--;

        Delta = (double)1.0/(double)divisor;

        t = 0.0;

        Done = FALSE;

        VP1.x = CPoints[Current].Vpt.x;
        VP1.y = CPoints[Current].Vpt.y;
        VP1.z = CPoints[Current].Vpt.z;
        VP2.x = CPoints[Next].Vpt.x;
        VP2.y = CPoints[Next].Vpt.y;
        VP2.z = CPoints[Next].Vpt.z;
        VtempPnt1.x = CPoints[Current].DVpt.x;
        VtempPnt1.y = CPoints[Current].DVpt.y;
        VtempPnt1.z = CPoints[Current].DVpt.z;
        VtempPnt2.x = CPoints[Next].DVpt.x;
        VtempPnt2.y = CPoints[Next].DVpt.y;
        VtempPnt2.z = CPoints[Next].DVpt.z;


#ifdef DEBUG
fprintf(DebugFile,"ctrlvec1.x.y.z=",
VtempPnt1.X:4:2,",",
VtempPnt1.Y:4:2,",",
VtempPnt1.Z:4:2);
fprintf(DebugFile,"ctrlvec2.x.y.z=",
VtempPnt2.X:4:2,",",
VtempPnt2.Y:4:2,",",
VtempPnt2.Z:4:2);
Close(DebugFile);
Append(DebugFile);
#endif


                if ( FirstTime && !Done)
                    FirstTime = FALSE;

#ifdef DEBUG
fprintf(DebugFile,"    Hermite - Delta=",Delta:10:8,
    "x1,y1,x2,y2=", VP1.X:4,",",vp1.Y:4,",",vp2.X:4,",",vp2.y:4 );
Close(DebugFile);
Append(DebugFile);
#endif

                while( !Done )
                    {
                    t2 = t*t; // squared
                    t3 = t2*t; // cubed 

                    C1 = (2*t3 - 3*t2 +     1);
                    C2 = (-2*t3 + 3*t2        );
                    C3 = (   t3 - 2*t2 + t    );
                    C4 = (   t3 -   t2        );

                    VtempPoint.x = VP1.x * C1 +
                                   VP2.x * C2 +
                                   VtempPnt1.x * C3 +
                                   VtempPnt2.x * C4;

                    VtempPoint.y = VP1.y * C1 +
                                   VP2.y * C2 +
                                   VtempPnt1.y * C3 +
                                   VtempPnt2.y * C4;

                    VtempPoint.z = VP1.z * C1 +
                                   VP2.z * C2 +
                                   VtempPnt1.z * C3 +
                                   VtempPnt2.z * C4;

#ifdef DEBUG
fprintf(debugfile,  "    V3DPoint in VMHermite X = ",VtempPoint.X:4:2, " Y = ",VtempPoint.Y:4:2, " Z = ",VtempPoint.Z:4:2 );
fprintf(debugfile,  "    L3DPoint in VMHermite X = ",LtempPoint.X:4:2, " Y = ",LtempPoint.Y:4:2, " Z = ",LtempPoint.Z:4:2 );
Close(debugfile);Append(debugfile);
#endif

                    VPoints[PointCnt++] = VtempPoint;

                    t += Delta;

                    if (Next == CtrlCnt-1) //Last curve segment include t=1
                        Done = ( t >= (0.999999+Delta));
                    else
                        Done = ( t >= 0.999999 ); // 1.0 is end of curve segment

                    } // while t

            if( Next < CtrlCnt-1 )// new control records...
                {
                Current++;
                Next++;
                }
            else
                Completed = TRUE;
            } // while more control points
    }


// ------- Main -----------


int main( int argc, char *argv[] )
    {

#ifdef DEBUG
Assign( DebugFile, "HermDBG.txt" );
ReWrite(DebugFile);
#endif

    CtrlCnt = 0;
    PointCnt = 0;

    if( argc > 1 )
        strcpy_s( InputFileName, argv[1] );
    else
        {
        memset( InputFileName, 0x00, sizeof(InputFileName) );
        printf("Enter Input Filename:");
        gets_s(InputFileName);
        printf("\n\n");
        }

    if (argc > 2 )
        strcpy_s( OutputFileName, argv[2] );
    else
        {
        memset( OutputFileName, 0x00, sizeof(OutputFileName) );
        printf("Enter Output filename:");
        gets_s(OutputFileName);
        printf("\n\n");
        }

    InFile = fopen(InputFileName, "r" );
    if (!InFile)
        {
        perror("Opening Input");
        exit(0);
        }

    OutFile = fopen(OutputFileName, "w" );
    if (!OutFile)
        {
        perror("Opening Output");
        exit(0);
        }

    Read_Input();
    fclose(InFile);

    Hermite3D();

    Start_Output();

    for (i=0; i<PointCnt; i++)
        Write_Output( VPoints[i], i );

    End_Output();

    fclose(OutFile);

    return(1);
    } // main


