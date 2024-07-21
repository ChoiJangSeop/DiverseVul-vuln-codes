int LibRaw::dcraw_process(void)
{
    int quality,i;

    int iterations=-1, dcb_enhance=1, noiserd=0;
    int eeci_refine_fl=0, es_med_passes_fl=0;
    float cared=0,cablue=0;
    float linenoise=0; 
    float lclean=0,cclean=0;
    float thresh=0;
    float preser=0;
    float expos=1.0;


    CHECK_ORDER_LOW(LIBRAW_PROGRESS_LOAD_RAW);
//    CHECK_ORDER_HIGH(LIBRAW_PROGRESS_PRE_INTERPOLATE);

    try {

        int no_crop = 1;

        if (~O.cropbox[2] && ~O.cropbox[3])
            no_crop=0;

        libraw_decoder_info_t di;
        get_decoder_info(&di);

        int subtract_inline = !O.bad_pixels && !O.dark_frame && !O.wf_debanding && !(di.decoder_flags & LIBRAW_DECODER_LEGACY) && !IO.zero_is_bad;

        raw2image_ex(subtract_inline); // allocate imgdata.image and copy data!

        int save_4color = O.four_color_rgb;

        if (IO.zero_is_bad) 
          {
                remove_zeroes();
                SET_PROC_FLAG(LIBRAW_PROGRESS_REMOVE_ZEROES);
            }

        if(O.half_size) 
            O.four_color_rgb = 1;

        if(O.bad_pixels && no_crop) 
            {
                bad_pixels(O.bad_pixels);
                SET_PROC_FLAG(LIBRAW_PROGRESS_BAD_PIXELS);
            }

        if (O.dark_frame && no_crop)
            {
                subtract (O.dark_frame);
                SET_PROC_FLAG(LIBRAW_PROGRESS_DARK_FRAME);
            }

	if (O.wf_debanding)
	{
		wf_remove_banding();
	}

        quality = 2 + !IO.fuji_width;

        if (O.user_qual >= 0) quality = O.user_qual;

        if(!subtract_inline || !C.data_maximum)
          {
            adjust_bl();
            subtract_black();
          }

        adjust_maximum();

        if (O.user_sat > 0) C.maximum = O.user_sat;

        if (P1.is_foveon) 
            {
              if(load_raw == &LibRaw::foveon_dp_load_raw)
                {
                  for (int i=0; i < S.height*S.width*4; i++)
                    if ((short) imgdata.image[0][i] < 0) imgdata.image[0][i] = 0;
                }
              else
                foveon_interpolate();
                SET_PROC_FLAG(LIBRAW_PROGRESS_FOVEON_INTERPOLATE);
            }

        if (O.green_matching && !O.half_size)
            {
                green_matching();
            }

        if (!P1.is_foveon)
            {
                scale_colors();
                SET_PROC_FLAG(LIBRAW_PROGRESS_SCALE_COLORS);
            }

        pre_interpolate();

        SET_PROC_FLAG(LIBRAW_PROGRESS_PRE_INTERPOLATE);

        if (O.dcb_iterations >= 0) iterations = O.dcb_iterations;
        if (O.dcb_enhance_fl >=0 ) dcb_enhance = O.dcb_enhance_fl;
        if (O.fbdd_noiserd >=0 ) noiserd = O.fbdd_noiserd;
        if (O.eeci_refine >=0 ) eeci_refine_fl = O.eeci_refine;
        if (O.es_med_passes >0 ) es_med_passes_fl = O.es_med_passes;

// LIBRAW_DEMOSAIC_PACK_GPL3

        if (!O.half_size && O.cfa_green >0) {thresh=O.green_thresh ;green_equilibrate(thresh);} 
        if (O.exp_correc >0) {expos=O.exp_shift ; preser=O.exp_preser; exp_bef(expos,preser);} 
        if (O.ca_correc >0 ) {cablue=O.cablue; cared=O.cared; CA_correct_RT(cablue, cared);}
        if (O.cfaline >0 ) {linenoise=O.linenoise; cfa_linedn(linenoise);}
        if (O.cfa_clean >0 ) {lclean=O.lclean; cclean=O.cclean; cfa_impulse_gauss(lclean,cclean);}

        if (P1.filters) 
            {
                if (noiserd>0 && P1.colors==3 && P1.filters) fbdd(noiserd);

                if (quality == 0)
                    lin_interpolate();
                else if (quality == 1 || P1.colors > 3 || P1.filters < 1000)
                    vng_interpolate();
                else if (quality == 2)
                    ppg_interpolate();

                else if (quality == 3) 
                    ahd_interpolate(); // really don't need it here due to fallback op

                else if (quality == 4)
                    dcb(iterations, dcb_enhance);

//  LIBRAW_DEMOSAIC_PACK_GPL2                
                else if (quality == 5)
                    ahd_interpolate_mod();
                else if (quality == 6)
                    afd_interpolate_pl(2,1);
                else if (quality == 7)
                    vcd_interpolate(0);
                else if (quality == 8)
                    vcd_interpolate(12);
                else if (quality == 9)
                    lmmse_interpolate(1);

// LIBRAW_DEMOSAIC_PACK_GPL3
                else if (quality == 10)
                    amaze_demosaic_RT();
// LGPL2
                else if (quality == 11)
                    dht_interpolate();
                else if (quality == 12)
                    aahd_interpolate();
 // fallback to AHD
                else
                    ahd_interpolate();
                
                SET_PROC_FLAG(LIBRAW_PROGRESS_INTERPOLATE);
            }
        if (IO.mix_green)
            {
                for (P1.colors=3, i=0; i < S.height * S.width; i++)
                    imgdata.image[i][1] = (imgdata.image[i][1] + imgdata.image[i][3]) >> 1;
                SET_PROC_FLAG(LIBRAW_PROGRESS_MIX_GREEN);
            }

        if(!P1.is_foveon)
            {
                if (P1.colors == 3) 
                    {
                        
                        if (quality == 8) 
                            {
                                if (eeci_refine_fl == 1) refinement();
                                if (O.med_passes > 0)    median_filter_new();
                                if (es_med_passes_fl > 0) es_median_filter();
                            } 
                        else {
                            median_filter();
                        }
                        SET_PROC_FLAG(LIBRAW_PROGRESS_MEDIAN_FILTER);
                    }
            }
        
        if (O.highlight == 2) 
            {
                blend_highlights();
                SET_PROC_FLAG(LIBRAW_PROGRESS_HIGHLIGHTS);
            }
        
        if (O.highlight > 2) 
            {
                recover_highlights();
                SET_PROC_FLAG(LIBRAW_PROGRESS_HIGHLIGHTS);
            }
        
        if (O.use_fuji_rotate) 
            {
                fuji_rotate();
                SET_PROC_FLAG(LIBRAW_PROGRESS_FUJI_ROTATE);
            }
    
        if(!libraw_internal_data.output_data.histogram)
            {
                libraw_internal_data.output_data.histogram = (int (*)[LIBRAW_HISTOGRAM_SIZE]) malloc(sizeof(*libraw_internal_data.output_data.histogram)*4);
                merror(libraw_internal_data.output_data.histogram,"LibRaw::dcraw_process()");
            }
#ifndef NO_LCMS
	if(O.camera_profile)
            {
                apply_profile(O.camera_profile,O.output_profile);
                SET_PROC_FLAG(LIBRAW_PROGRESS_APPLY_PROFILE);
            }
#endif

        convert_to_rgb();
        SET_PROC_FLAG(LIBRAW_PROGRESS_CONVERT_RGB);

        if (O.use_fuji_rotate) 
            {
                stretch();
                SET_PROC_FLAG(LIBRAW_PROGRESS_STRETCH);
            }
        O.four_color_rgb = save_4color; // also, restore

        return 0;
    }
    catch ( LibRaw_exceptions err) {
        EXCEPTION_HANDLER(err);
    }
}