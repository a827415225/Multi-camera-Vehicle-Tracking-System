#include "header.hpp"
#include "bsgmm.hpp"
#include "rect.hpp"
#include "ptrans.hpp"

int main( int argc, char *argv[] )
{

    // codes for control command line options {{{

    bool aviOutput = false;
    bool maskOutput = false;
    int fastforward = 0;
    int  options;
    string videoOutPath, maskOutPath, inputPath;

    if ( argc == 1 )
    {
        cout << "usage: ./BSGMM [options]" << endl;
        cout << "options:" << endl;
        cout << "-i [input video path]  (required)" << endl;
        cout << "-v [output video path]" << endl;
        cout << "-m [mask video output path]" << endl;
        cout << "-t [video start time (secs)]" << endl;
        exit( EXIT_FAILURE );
    }
    struct option  long_opt[] =
    {
        {"input", required_argument, NULL, 'i'},
        {"mask", required_argument, NULL, 'm'},
        {"video", required_argument, NULL, 'v'},
        {"time", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}
    };
    while ( ( options = getopt_long( argc, argv, "i:m:v:t:", long_opt, NULL ) ) != -1 )
    {
        switch  ( options )
        {
        case 'i':
            inputPath = string( optarg );
            break;
        case 'm':
            maskOutPath = string( optarg );
            maskOutput = true;
            break;
        case 'v':
            aviOutput = true;
            videoOutPath = string( optarg );
            break;
        case 't':
            fastforward = atoi( optarg );
            break;
        }
    }

    // }}}

    // {{{ declare mat for input and mask

    cv::Mat inputImg, outputMask;
    cv::Size newSize( 800, 450 );
    cv::VideoCapture capture( inputPath );
    // perform fast foward
    capture.set( CV_CAP_PROP_POS_FRAMES, fastforward * FPS );
    if ( !capture.read( inputImg ) )
    {
        cout << " Can't recieve input from source " << endl;
        exit( EXIT_FAILURE );
    }
    cv::resize( inputImg, inputImg, newSize );
    outputMask = cv::Mat( inputImg.size(), CV_8UC1, BLACK_C1 );
    // }}}

    //declare output stream{{{

    cv::VideoWriter writer, writer2;
    if ( aviOutput )
    {
        writer.open( videoOutPath, CV_FOURCC( 'D', 'I', 'V', 'X' ), FPS,
                     cv::Size( capture.get( CV_CAP_PROP_FRAME_WIDTH ), capture.get( CV_CAP_PROP_FRAME_HEIGHT ) ) );
    }
    if ( maskOutput )
    {
        writer2.open( maskOutPath, CV_FOURCC( 'D', 'I', 'V', 'X' ), FPS,
                      cv::Size( capture.get( CV_CAP_PROP_FRAME_WIDTH ), capture.get( CV_CAP_PROP_FRAME_HEIGHT ) ) );
    }
    //}}}

    // {{{creat rotation matrix

    perspectiveTransform ptrans;
    /* kymco */
    /* ptrans.setSrcPts( cv::Point2f( 370, 190 ), cv::Point2f( 0, 230 ), cv::Point2f( 650, 410 ), cv::Point2f( 780, 225 ) ); */
    /* 711 */
    ptrans.setSrcPts( cv::Point2f( 330, 100 ), cv::Point2f( 0, 180 ), cv::Point2f( 730, 390 ), cv::Point2f( 660, 145 ) );
    ptrans.setDstPts( cv::Point2f( 300, 20 ), cv::Point2f( 300, 420 ), cv::Point2f( 700, 420 ), cv::Point2f( 700, 20 ) );
    cv::Mat perspective_matrix = ptrans.getMatrix();

    // }}}

    // {{{creat GMM Class object

    BackgroundSubtractorGMM bsgmm(  inputImg.rows, inputImg.cols );
    bsgmm.shadowBeBackground = true;

    // }}}

    while ( capture.read( inputImg ) )
    {
        cv::resize( inputImg, inputImg, newSize );
        /* cv::Mat inputBlur; */
        /* cv::GaussianBlur( inputImg, inputBlur, cv::Size( 5, 5 ), 0, 0 ); */
        /* bsgmm.updateFrame( inputBlur.ptr(), outputMask.ptr() ); */
        bsgmm.updateFrame( inputImg.ptr(), outputMask.ptr() );
        cv::Mat outputMorp;
        cv::morphologyEx( outputMask, outputMorp, CV_MOP_CLOSE, getStructuringElement( cv::MORPH_RECT, cv::Size( 5, 5 ) ) );

        // draw rect print words on img for debug {{{
        findRect rect( inputImg, outputMorp );
        vector<cv::Rect> boundRect =  rect.findBoundingRect();
        vector<cv::Point2f> ori;

        for ( unsigned int i = 0; i < boundRect.size(); i++ )
        {
            rectangle( inputImg, boundRect[i].tl(), boundRect[i].br(), RED_C3, 2 );
            cv::Point2f mapPts( boundRect[i].x + boundRect[i].width / 2, boundRect[i].y + boundRect[i].height * 0.75 );
            cv::circle( inputImg, mapPts, 4 , BLUE_C3, CV_FILLED );
            ori.push_back( mapPts );
        }

        string str = "Count:" + to_string( boundRect.size() ) + " Frame:" + to_string( ( int )capture.get( CV_CAP_PROP_POS_FRAMES ) ) + "time:" + to_string( ( int )( capture.get( CV_CAP_PROP_POS_FRAMES ) / FPS ) );
        putText( inputImg, str, cv::Point( 300, inputImg.rows - 20 ), cv::FONT_HERSHEY_PLAIN, 2,  RED_C3, 2 );
        //}}}

        cv::imshow( "video", inputImg );
        /* cv::imshow( "GMM", outputMask ); */
        /* cv::imshow( "inputBlur", inputBlur ); */
        /* cv::imshow( "outputMorp", outputMorp ); */

        cv::Mat roadMap( cv::Size( 600, 600 ), inputImg.type(), WHITE_C3 );
        cv::line( roadMap, cv::Point( 100, 100 ), cv::Point( 510, 100 ), BLUE_C3, 2 );
        cv::line( roadMap, cv::Point( 510, 100 ), cv::Point( 510, 510 ), BLUE_C3, 2 );
        cv::line( roadMap, cv::Point( 510, 510 ), cv::Point( 100, 510 ), BLUE_C3, 2 );
        cv::line( roadMap, cv::Point( 100, 510 ), cv::Point( 100, 100 ), BLUE_C3, 2 );
        if ( boundRect.size() > 0 )
        {
            vector<cv::Point2f> dst;
            cv::perspectiveTransform( ori, dst, perspective_matrix );
            for ( unsigned int i = 0; i < dst.size(); i++ )
            {
                cv::circle( roadMap, cv::Point( dst[i].x - 300 + 100 , dst[i].y - 20 + 100  ), 10 , RED_C3, CV_FILLED );
            }
        }
        cv::imshow( "roadMap", roadMap );


        // write to avi{{{
        if ( aviOutput )
        {
            writer << inputImg;
        }
        if ( maskOutput )
        {
            cv::Mat maskForAvi;
            cv::cvtColor( outputMorp, maskForAvi, CV_GRAY2RGB );
            //because our mask is single channel, we need to convert it to three channel to output avi
            putText( maskForAvi , str, cv::Point( 300, inputImg.rows - 20 ), cv::FONT_HERSHEY_PLAIN, 2,  RED_C3, 2 );
            writer2 << maskForAvi;
        }
        //}}}

        // monitor keys to stop{{{
        if ( cv::waitKey( 1 ) > 0 )
        {
            break;
        }
        //}}}
    }
    bsgmm.freeMem();
    return EXIT_SUCCESS;
}
