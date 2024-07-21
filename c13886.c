lp8000_print_page(gx_device_printer *pdev, gp_file *prn_stream)
{

        int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
        int in_size = line_size;
        unsigned int report_size;

        byte *buf1 = (byte *)gs_malloc(pdev->memory, in_size, 1, "lp8000_print_page(buf1)");
        byte *buf2 = (byte *)gs_malloc(pdev->memory, in_size, 1, "lp8000_print_page(buf2)");
        byte *in = buf1;
        byte *out = buf2;

        int lnum, top, bottom, left, width;
        int count, left1, left2, left0;

/* Check  memory allocations  */

        if ( buf1 == 0 || buf2 == 0 )
        {       if ( buf1 )
                gs_free(pdev->memory, (char *)buf1, in_size, 1, "lp8000_print_page(buf1)");

                if ( buf2 )
                gs_free(pdev->memory, (char *)buf2, in_size, 1, "lp8000_print_page(buf2)");

          return_error(gs_error_VMerror);
        }

/* Initialize the printer */

        gp_fwrite("\033\001@EJL \n",1,8,prn_stream);
        gp_fwrite("@EJL EN LA=ESC/PAGE\n",1,20,prn_stream);
        gp_fwrite("\035rhE\033\001@EJL \n",1,12,prn_stream);
        gp_fwrite("@EJL SE LA=ESC/PAGE\n",1,20,prn_stream);
        gp_fwrite("@EJL SET PU=1 PS=A4 ZO=OFF\n",1,27,prn_stream);
        gp_fwrite("@EJL EN LA=ESC/PAGE\n",1,20,prn_stream);
        gp_fwrite("\0350;0.24muE\0352;300;300drE",1,23,prn_stream);
        gp_fwrite("\0350;300;300drE\0351tsE\0351mmE",1,23,prn_stream);
        gp_fwrite("\0357isE\0355iaF\0355ipP\03514psE\0350poE",1,26,prn_stream);
        gp_fwrite("\03560;60loE\0350X\0350Y",1,15,prn_stream);
        gp_fwrite("\0350;0;2360;3388caE",1,17,prn_stream);
        gp_fwrite("\0351cmE\0350alfP",1,11,prn_stream);
        gp_fwrite("\0350affP\0350boP\0350abP",1,16,prn_stream);
        gp_fwrite("\0354ilG\0350bcI\0350sarG",1,16,prn_stream);
        gp_fwrite("\0351;0;100spE\0352owE",1,16,prn_stream);

/* Here the common part of the initialization string ends */

/* Calculate the PRINTER_LEFT_MARGIN = device_left_margin - 60 adjusted to
the byte boundary. Save this value for future comparison and set the
starting X value of the printer line.
*/
        left1  =  (int) (L_MARGIN * pdev->x_pixels_per_inch) - 60;
        left1 = (left1 >> 3) << 3;
        left0 = left1;

        gp_fwrite("\035",1,1,prn_stream);
        gp_fprintf(prn_stream,"%d",left1);
        gp_fwrite("X",1,1,prn_stream);

        /* Set the compressed data format */
        gp_fwrite("\0353bcI",1,5,prn_stream);

        top = T_MARGIN * pdev->y_pixels_per_inch;
        bottom = pdev->height - B_MARGIN * pdev->y_pixels_per_inch;

        left  = ( (int) (L_MARGIN * pdev->x_pixels_per_inch) ) >> 3 ;
        width = ((pdev->width - (int)(R_MARGIN * pdev->x_pixels_per_inch)) >> 3) - left;

        /*
        ** Print the page:
        */

        for ( lnum = top; lnum < bottom ; )

        {
                byte *in_data;
                byte *inp;
                byte *in_end;
                byte *outp;
                register byte *p, *q;

                /*
                ** Check buffer for 0 data.
                */

                gdev_prn_get_bits(pdev, lnum, in, &in_data);
                while ( in_data[0] == 0 &&
                        !memcmp((char *)in_data, (char *)in_data + 1, line_size - 1) &&
                        lnum < bottom )
                {
                        lnum++;
                        gdev_prn_get_bits(pdev, lnum, in, &in_data);
                }

                if(lnum == bottom ) break;
                /* finished with this page */

                (void)gdev_prn_copy_scan_lines(pdev, lnum, in, in_size);

                inp = in  + left;
                in_end = inp + width;

/* Remove trailing 0s form the scan line data */

                while (in_end > inp &&  in_end[-1] == 0)
                {
                in_end--;
                }

/* Remove leading 0s form the scan line data */

        for(left2 = 0; inp < in_end && inp[0] == 0; inp++,left2++);

/* Recalculate starting X value */

        left2 = left1 + (left2 << 3);

/* Compress  non-zero data for this line*/

                outp = out;

 for( p = inp, q = inp + 1 ; q < in_end ; )
        {
        if( *p != *q++ )
          {
          /*
          Copy non-repeated bytes
          to the output buffer
          */
          *outp++ = *p++;
          }
          else
            {
                for (count = 2; ( *p == *q ) && (q < in_end); q++, count++)
                {}

                /*
                Copy repeated bytes and counts to the output buffer.
                As long as count is <= 255, additional step is necessary
                for a long repeated sequence
                */

                while (count > 257)
                {
                *outp++ = *p;
                *outp++ = *p;
                *outp++ = 255;
                p += 257;
                count -=257;
                }
                *outp++ = *p;
                *outp++ = *p;
                *outp++ = count - 2;
                p += count;
                q = p+1;
            }
        }

/* The next line is necessary just in case of a single non-repeated byte at
the end of the input buffer */

if (p == (in_end - 1)) *outp++ = *p;

/* End of the compression procedure */

/* Set a new value of the starting X point, if necessary  */

if (left2 != left0)
        {
        left0 = left2;
        gp_fwrite("\035",1,1,prn_stream);
        gp_fprintf(prn_stream,"%d",left2);
        gp_fwrite("X",1,1,prn_stream);
        }

/* Output the data string to the printer.
Y coordinate of the printer equals (lnum - 60)
*/

        gp_fwrite("\035",1,1,prn_stream);
        gp_fprintf(prn_stream,"%d",lnum-60);
        gp_fwrite("Y\035",1,2,prn_stream);
        report_size = outp - out;
        gp_fprintf(prn_stream,"%d;",report_size);
        report_size = (in_end - inp) << 3;
        gp_fprintf(prn_stream,"%d;",report_size);
        gp_fwrite("1;0bi{I",1,7,prn_stream);
        gp_fwrite(out,1,(outp - out),prn_stream);

        lnum++;

        }

/* Send the termination string */

        gp_fwrite("\0350bcI",1,5,prn_stream);
        gp_fwrite("\0351coO",1,5,prn_stream);
        gp_fwrite("\035rhE",1,4,prn_stream);

        gp_fwrite("\033\001@EJL \n",1,8,prn_stream);
        gp_fwrite("@EJL SE LA=ESC/PAGE\n",1,20,prn_stream);
        gp_fwrite("@EJL SET PU=1 PS=A4 ZO=OFF\n",1,27,prn_stream);
        gp_fwrite("@EJL EN LA=ESC/PAGE\n",1,20,prn_stream);
        gp_fwrite("\0350;0.24muE\0352;300;300drE",1,23,prn_stream);
        gp_fwrite("\0350;300;300drE\0351tsE\0351mmE",1,23,prn_stream);
        gp_fwrite("\0357isE\0355iaF\0355ipP\03514psE\0350poE",1,26,prn_stream);
        gp_fwrite("\03560;60loE\0350X\0350Y",1,15,prn_stream);
        gp_fwrite("\0350;0;2360;3388caE",1,17,prn_stream);
        gp_fwrite("\0351cmE\0350alfP",1,11,prn_stream);
        gp_fwrite("\0350affP\0350boP\0350abP",1,16,prn_stream);
        gp_fwrite("\0354ilG\0350bcI\0350sarG",1,16,prn_stream);
        gp_fwrite("\035rhE",1,4,prn_stream);
        gp_fwrite("\033\001@EJL \n",1,8,prn_stream);
        gp_fwrite("\033\001@EJL \n",1,8,prn_stream);

        gp_fflush(prn_stream);

        gs_free(pdev->memory, (char *)buf2, in_size, 1, "lp8000_print_page(buf2)");
        gs_free(pdev->memory, (char *)buf1, in_size, 1, "lp8000_print_page(buf1)");
        return 0;
}