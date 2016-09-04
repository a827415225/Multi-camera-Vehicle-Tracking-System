#include "rect.hpp"

findRect::findRect( cv::Mat &inputImg, cv::Mat &mask ) : inputImg( inputImg ), mask( mask )
{}

cv::Rect findRect::removeShadowRect ( cv::Rect rect )
{
    int w = 0, h = 0;
    if ( rect.width > rect.height * 2 )
    {
        w = 16;
        h = 8;
    }
    else if ( rect.width * 2 < rect.height )
    {
        w = 8;
        h = 16;
    }
    else
    {
        w = 8;
        h = 8;
    }
    int rw = rect.width;
    int rh = rect.height;
    int tlx = rect.x;
    int tly = rect.y;
    int xbase = rect.tl().x + rect.width / ( 2 * w );
    int ybase = rect.tl().y + rect.height / ( 2 * h );
    for ( int i = 0, leftSideCutCnt = 0; i < w ; i++ )
    {
        int numOfBGSquare = 0;
        for ( int j = 0; j < h; j++ )
        {
            if ( ( uint )mask.at<uchar>( ybase + j * ( rect.height / h ), xbase + i * ( rect.width / w ) ) == 255 )
            {
                /* cv::circle( inputImg, cv::Point( xbase + i * ( rect.width / w ),  ybase + j * ( rect.height / h ) ), 1, BLACK_C3, 2 ); */
            }
            else
            {
                /* cv::circle( inputImg, cv::Point( xbase + i * ( rect.width / w ),  ybase + j * ( rect.height / h ) ), 1, WHITE_C3, 2 ); */
                numOfBGSquare++;
            }
        }
        if ( numOfBGSquare >= ( int )( 0.75 * h )  )
        {
            if ( leftSideCutCnt == i )
            {
                tlx += rect.width / w;
                leftSideCutCnt++;
            }
            rw -= rect.width / w;
        }
    }
    return cv::Rect( tlx, tly, rw, rh );
}

vector<cv::Rect> findRect::findBoundingRect()
{
    vector<vector<cv::Point>> contours;
    vector<cv::Vec4i> hierarchy;
    cv::Mat tmp;
    mask.copyTo( tmp );
    findContours( tmp, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
    vector<vector<cv::Point>> contours_poly( contours.size() );
    vector<cv::Rect> boundRect;
    int whitePixelCnt = 0;
    for ( int i = 0; i < mask.rows; i++ )
    {
        for ( int j = 0; j < mask.cols; j++ )
        {
            if ( mask.at<uchar>( i, j ) == ( uchar )255 )
            {
                whitePixelCnt++;
            }
        }
    }
    int percentage = ( int )( ( ( double )whitePixelCnt / ( double )( mask.cols * mask.rows ) ) * 100 );
    if ( percentage >= 30 )
    {
        putText( inputImg, "Warning:burst light", cv::Point( 50, 50 ), cv::FONT_HERSHEY_PLAIN, 2,  RED_C3, 2 );
        return boundRect;
    }
    for ( unsigned int i = 0; i < contours.size(); i++ )
    {
        approxPolyDP( cv::Mat( contours[i] ), contours_poly[i], 3, true );
        if ( cv::contourArea( contours_poly[i] ) > 300 )
        {
            cv::Rect newRect = boundingRect( cv::Mat( contours_poly[i] ) );
            /* cv::drawContours( inputImg, contours_poly, i, GREEN_C3, 2 ); */
            /* cv::Moments mo = moments( contours_poly[i] ); */
            /* cv::Point center = cv::Point( mo.m10 / mo.m00 , mo.m01 / mo.m00 ); */
            /* cv::drawContours( mask, contours_poly, i, WHITE_C1, CV_FILLED ); */
            boundRect.push_back( this->removeShadowRect( newRect ) );
        }
    }
    return boundRect;
}