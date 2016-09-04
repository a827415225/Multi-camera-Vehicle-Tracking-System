#include "header.hpp"
#include "bsgmm.hpp"
#include "rect.hpp"
#include "ptrans.hpp"
#include "gnuplot-iostream.h"

int main( int argc, char *argv[] )
{
    /* {{{ global variable set by commad line parameters */
    int fastforward = 0;
    int  options;
    string videoOutPath, maskOutPath, inputPath711, inputPathKymco, outputPath;
    /* }}} */

    // codes for control command line options {{{


    if ( argc == 1 )
    {
        cout << "usage: ./bsgmm711 [options]" << endl;
        cout << "options:" << endl;
        cout << "-i [input 711 video path]  (required)" << endl;
        cout << "-j [input kymco video path]  (required)" << endl;
        cout << "-m [output video path]  (required)" << endl;
        cout << "-t [video start time (secs)]" << endl;
        exit( EXIT_FAILURE );
    }
    struct option  long_opt[] =
    {
        {"input", required_argument, NULL, 'i'},
        {"input2", required_argument, NULL, 'j'},
        {"output", required_argument, NULL, 'm'},
        {"time", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}
    };
    while ( ( options = getopt_long( argc, argv, "i:j:m:t:", long_opt, NULL ) ) != -1 )
    {
        switch  ( options )
        {
        case 'i':
            inputPath711 = string( optarg );
            break;
        case 'j':
            inputPathKymco = string( optarg );
            break;
        case 'm':
            outputPath = string( optarg );
            break;
        case 't':
            fastforward = atoi( optarg );
            break;
        }
    }

    // }}}

    // {{{ global variable declaration

    cv::Mat inputImg711, outputMask711;
    cv::Mat inputImgKymco, outputMaskKymco;
    cv::Size newSize( 800, 450 );
    cv::VideoCapture capture711( inputPath711 );
    cv::VideoCapture captureKymco( inputPathKymco );
    // perform fast foward
    capture711.set( CV_CAP_PROP_POS_FRAMES, fastforward * FPS );
    if ( !capture711.read( inputImg711 ) )
    {
        cout << " Can't recieve input from source " << endl;
        exit( EXIT_FAILURE );
    }
    captureKymco.set( CV_CAP_PROP_POS_FRAMES, fastforward * FPS );
    if ( !captureKymco.read( inputImgKymco ) )
    {
        cout << " Can't recieve input from source " << endl;
        exit( EXIT_FAILURE );
    }
    /* cv::resize( inputImg711, inputImg711, newSize ); */
    /* cv::resize( inputImgKymco, inputImgKymco, newSize ); */
    outputMask711 = cv::Mat( inputImg711.size(), CV_8UC1, BLACK_C1 );
    outputMaskKymco = cv::Mat( inputImgKymco.size(), CV_8UC1, BLACK_C1 );
    cv::Mat originRoadMap( cv::Size( 600, 600 ), inputImg711.type(), GRAY_C3 );
    cv::Mat merge = cv::Mat::zeros( inputImg711.rows * 2 + 5, inputImg711.cols + originRoadMap.cols + 5, inputImg711.type() );
    // }}}

    // {{{creat rotation matrix

    perspectiveTransform ptrans711;
    ptrans711.setSrcPts( cv::Point2f( 330, 95 ), cv::Point2f( 0, 180 ), cv::Point2f( 745, 410 ), cv::Point2f( 665, 145 ) );
    ptrans711.setDstPts( cv::Point2f( 300, 20 ), cv::Point2f( 300, 420 ), cv::Point2f( 700, 420 ), cv::Point2f( 700, 20 ) );
    cv::Mat perspective_matrix711 = ptrans711.getMatrix();

    perspectiveTransform ptransKymco;
    ptransKymco.setSrcPts( cv::Point2f( 370, 180 ), cv::Point2f( 0, 230 ), cv::Point2f( 650, 415 ), cv::Point2f( 780, 225 ) );
    ptransKymco.setDstPts( cv::Point2f( 300, 20 ), cv::Point2f( 300, 420 ), cv::Point2f( 700, 420 ), cv::Point2f( 700, 20 ) );
    cv::Mat perspective_matrixKymco = ptransKymco.getMatrix();
    // }}}

    /* {{{create originroadMap background */

    cv::rectangle( originRoadMap, cv::Point( 100, 100 ), cv::Point( 500, 500 ), BLUE_C3, 2 );
    for ( int i = 1; i <= 10; i++ )
    {
        cv::rectangle( originRoadMap, cv::Point( 10, 100 + 35 * i ), cv::Point( 90, 120 + 35 * i ), WHITE_C3, CV_FILLED );
        cv::rectangle( originRoadMap, cv::Point( 510, 100 + 35 * i ), cv::Point( 590, 120 + 35 * i ), WHITE_C3, CV_FILLED );
        cv::rectangle( originRoadMap, cv::Point( 100 + 35 * i, 10 ), cv::Point( 120 + 35 * i, 90 ), WHITE_C3, CV_FILLED );
        cv::rectangle( originRoadMap, cv::Point( 100 + 35 * i, 510 ), cv::Point( 120 + 35 * i, 590 ), WHITE_C3, CV_FILLED );
    }
    putText( originRoadMap , "7-11", cv::Point( 20, 60 ), cv::FONT_HERSHEY_PLAIN, 2,  RED_C3, 2 );
    putText( originRoadMap , "KYMCO", cv::Point( 500, 540 ), cv::FONT_HERSHEY_PLAIN, 2,  RED_C3, 2 );

    /* }}} */

    // {{{creat GMM Class object

    BackgroundSubtractorGMM bsgmm711(  inputImg711.rows, inputImg711.cols );
    bsgmm711.shadowBeBackground = true;

    BackgroundSubtractorGMM bsgmmKymco(  inputImgKymco.rows, inputImgKymco.cols );
    bsgmmKymco.shadowBeBackground = true;

    // }}}

    while ( capture711.read( inputImg711 ) && captureKymco.read( inputImgKymco ) )
    {
        /* cv::resize( inputImg711, inputImg711, newSize ); */
        bsgmm711.updateFrame( inputImg711.ptr(), outputMask711.ptr() );
        cv::Mat outputMorp711;
        cv::morphologyEx( outputMask711, outputMorp711, CV_MOP_CLOSE, getStructuringElement( cv::MORPH_RECT, cv::Size( 5, 5 ) ) );

        /* cv::resize( inputImgKymco, inputImgKymco, newSize ); */
        bsgmmKymco.updateFrame( inputImgKymco.ptr(), outputMaskKymco.ptr() );
        cv::Mat outputMorpKymco;
        cv::morphologyEx( outputMaskKymco, outputMorpKymco, CV_MOP_CLOSE, getStructuringElement( cv::MORPH_RECT, cv::Size( 5, 5 ) ) );

        // draw rect print words on img for debug {{{
        findRect rect711( inputImg711, outputMorp711 );
        vector<cv::Rect> boundRect711 =  rect711.findBoundingRect();
        vector<cv::Point2f> ori711;

        for ( unsigned int i = 0; i < boundRect711.size(); i++ )
        {
            rectangle( inputImg711, boundRect711[i].tl(), boundRect711[i].br(), RED_C3, 2 );
            cv::Point2f mapPts( boundRect711[i].x + boundRect711[i].width / 2, boundRect711[i].y + boundRect711[i].height * 0.9 );
            cv::circle( inputImg711, mapPts, 4 , BLUE_C3, CV_FILLED );
            ori711.push_back( mapPts );
        }

        string str711 = "Count:" + to_string( boundRect711.size() ) + " Frame:" + to_string( ( int )capture711.get( CV_CAP_PROP_POS_FRAMES ) ) + "time:" + to_string( ( int )( capture711.get( CV_CAP_PROP_POS_FRAMES ) / FPS ) );
        putText( inputImg711, str711, cv::Point( 300, inputImg711.rows - 20 ), cv::FONT_HERSHEY_PLAIN, 2,  RED_C3, 2 );

        findRect rectKymco( inputImgKymco, outputMorpKymco );
        vector<cv::Rect> boundRectKymco =  rectKymco.findBoundingRect();
        vector<cv::Point2f> oriKymco;

        for ( unsigned int i = 0; i < boundRectKymco.size(); i++ )
        {
            rectangle( inputImgKymco, boundRectKymco[i].tl(), boundRectKymco[i].br(), RED_C3, 2 );
            cv::Point2f mapPts( boundRectKymco[i].x + boundRectKymco[i].width / 2, boundRectKymco[i].y + boundRectKymco[i].height * 0.9 );
            cv::circle( inputImgKymco, mapPts, 4 , BLUE_C3, CV_FILLED );
            oriKymco.push_back( mapPts );
        }

        string strKymco = "Count:" + to_string( boundRectKymco.size() ) + " Frame:" + to_string( ( int )captureKymco.get( CV_CAP_PROP_POS_FRAMES ) ) + "time:" + to_string( ( int )( capture711.get( CV_CAP_PROP_POS_FRAMES ) / FPS ) );
        putText( inputImgKymco, strKymco, cv::Point( 300, inputImg711.rows - 20 ), cv::FONT_HERSHEY_PLAIN, 2,  RED_C3, 2 );

        cv::Mat roadMap = originRoadMap.clone();

        if ( boundRect711.size() > 0 )
        {
            vector<cv::Point2f> dst;
            cv::perspectiveTransform( ori711, dst, perspective_matrix711 );
            for ( unsigned int i = 0; i < dst.size(); i++ )
            {
                int x = dst[i].x - 300 + 100;
                int y = dst[i].y - 20 + 100;
                if ( x + y >= 400 )
                {
                    cv::circle( roadMap, cv::Point( x, y ), 10 , RED_C3, CV_FILLED );
                }
            }
        }
        /* cv::imshow( "video711", inputImg711 ); */
        /* cv::imshow( "GMM711", outputMask711 ); */
        /* cv::imshow( "outputMorp711", outputMorp711 ); */

        if ( boundRectKymco.size() > 0 )
        {
            vector<cv::Point2f> dst;
            cv::perspectiveTransform( oriKymco, dst, perspective_matrixKymco );
            for ( unsigned int i = 0; i < dst.size(); i++ )
            {
                int x = abs( 600 - ( dst[i].x - 300 + 100 ) );
                int y = abs( 600 - ( dst[i].y - 20 + 100 ) );
                if ( x + y <= 400 )
                {
                    cv::circle( roadMap, cv::Point( x, y ), 10 , RED_C3, CV_FILLED );
                }
            }
        }
        /* cv::imshow( "videoKymco", inputImgKymco ); */
        /* cv::imshow( "GMMKymco", outputMaskKymco ); */
        /* cv::imshow( "outputMorpKymco", outputMorpKymco ); */

        /* cv::imshow( "roadMap", roadMap ); */

        merge.setTo( 0 );
        inputImg711.copyTo( merge( cv::Range( 0, newSize.height ) , cv::Range( 0, newSize.width ) ) );
        inputImgKymco.copyTo( merge( cv::Range( newSize.height + 5, newSize.height * 2 + 5 ) , cv::Range( 0, newSize.width ) ) );
        roadMap.copyTo( merge( cv::Range( 0, roadMap.rows ) , cv::Range( newSize.width + 5, roadMap.cols + newSize.width + 5 ) ) );

        cv::imshow( "merge", merge );

        // monitor keys to stop{{{
        if ( cv::waitKey( 1 ) > 0 )
        {
            break;
        }
        //}}}
    }
    bsgmm711.freeMem();
    return EXIT_SUCCESS;
}
