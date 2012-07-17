#!/bin/sh
#
# automatic regression test for libavformat
#
#
#set -x

set -e

. $(dirname $0)/regression-funcs.sh

eval do_$test=y

ENC_OPTS="$ENC_OPTS -metadata title=lavftest"

do_lavf_fate()
{
    file=${outfile}lavf.$1
    input="${samples}/$2"
    do_avconv $file $DEC_OPTS -i "$input" $ENC_OPTS -vcodec copy -acodec copy
    do_avconv_crc $file $DEC_OPTS -i $target_path/$file $3
}

do_lavf()
{
    file=${outfile}lavf.$1
    do_avconv $file $DEC_OPTS -f image2 -vcodec pgmyuv -i $raw_src $DEC_OPTS -ar 44100 -f s16le -i $pcm_src $ENC_OPTS -b:a 64k -t 1 -qscale:v 10 $2
    do_avconv_crc $file $DEC_OPTS -i $target_path/$file $3
}

do_lavf_timecode_nodrop() { do_lavf $1 "$2 -timecode 02:56:14:13"; }
do_lavf_timecode_drop()   { do_lavf $1 "$2 -timecode 02:56:14.13 -r 30000/1001"; }

do_lavf_timecode()
{
    do_lavf_timecode_nodrop "$@"
    do_lavf_timecode_drop "$@"
    do_lavf "$@"
}

do_streamed_images()
{
    file=${outfile}${1}pipe.$1
    do_avconv $file $DEC_OPTS -f image2 -vcodec pgmyuv -i $raw_src -f image2pipe $ENC_OPTS -t 1 -qscale 10
    do_avconv_crc $file $DEC_OPTS -f image2pipe -i $target_path/$file
}

do_image_formats()
{
    outfile="$datadir/images/$1/"
    mkdir -p "$outfile"
    file=${outfile}%02d.$1
    run_avconv $DEC_OPTS -f image2 -vcodec pgmyuv -i $raw_src $2 $ENC_OPTS $3 -t 0.5 -y -qscale 10 $target_path/$file
    do_md5sum ${outfile}02.$1
    do_avconv_crc $file $DEC_OPTS $3 -i $target_path/$file
    wc -c ${outfile}02.$1
}

do_audio_only()
{
    file=${outfile}lavf.$1
    do_avconv $file $DEC_OPTS $2 -ar 44100 -f s16le -i $pcm_src $ENC_OPTS -t 1 -qscale 10 $3
    do_avconv_crc $file $DEC_OPTS $4 -i $target_path/$file
}

if [ -n "$do_avi" ] ; then
do_lavf avi "-acodec mp2 -ab 64k"
fi

if [ -n "$do_asf" ] ; then
do_lavf asf "-acodec mp2 -ab 64k" "-r 25"
fi

if [ -n "$do_rm" ] ; then
file=${outfile}lavf.rm
do_avconv $file $DEC_OPTS -f image2 -vcodec pgmyuv -i $raw_src $DEC_OPTS -ar 44100 -f s16le -i $pcm_src $ENC_OPTS -t 1 -qscale 10 -acodec ac3_fixed -ab 64k
# broken
#do_avconv_crc $file -i $target_path/$file
fi

if [ -n "$do_mpg" ] ; then
do_lavf_timecode mpg "-ab 64k"
fi

if [ -n "$do_mxf" ] ; then
do_lavf_timecode mxf "-ar 48000 -bf 2"
fi

if [ -n "$do_mxf_d10" ]; then
do_lavf mxf_d10 "-ar 48000 -ac 2 -r 25 -s 720x576 -vf pad=720:608:0:32 -vcodec mpeg2video -g 0 -flags +ildct+low_delay -dc 10 -non_linear_quant 1 -intra_vlc 1 -qscale 1 -ps 1 -qmin 1 -rc_max_vbv_use 1 -rc_min_vbv_use 1 -pix_fmt yuv422p -minrate 30000k -maxrate 30000k -b 30000k -bufsize 1200000 -top 1 -rc_init_occupancy 1200000 -qmax 12 -f mxf_d10"
fi

if [ -n "$do_ts" ] ; then
do_lavf ts "-ab 64k -mpegts_transport_stream_id 42"
fi

if [ -n "$do_swf" ] ; then
do_lavf swf -an
fi

if [ -n "$do_ffm" ] ; then
do_lavf ffm "-ab 64k"
fi

if [ -n "$do_flv_fmt" ] ; then
do_lavf flv -an
fi

if [ -n "$do_mov" ] ; then
do_lavf_timecode mov "-acodec pcm_alaw -vcodec mpeg4"
fi

if [ -n "$do_ismv" ] ; then
do_lavf_timecode ismv "-an -vcodec mpeg4"
fi

if [ -n "$do_dv_fmt" ] ; then
do_lavf_timecode_nodrop dv "-ar 48000 -r 25 -s pal -ac 2"
do_lavf_timecode_drop   dv "-ar 48000 -pix_fmt yuv411p -s ntsc -ac 2"
do_lavf dv "-ar 48000 -r 25 -s pal -ac 2"
fi

if [ -n "$do_gxf" ] ; then
do_lavf_timecode_nodrop gxf "-ar 48000 -r 25 -s pal -ac 1"
do_lavf_timecode_drop   gxf "-ar 48000 -s ntsc -ac 1"
do_lavf gxf "-ar 48000 -r 25 -s pal -ac 1"
fi

if [ -n "$do_nut" ] ; then
do_lavf nut "-acodec mp2 -ab 64k"
fi

if [ -n "$do_mkv" ] ; then
do_lavf mkv "-acodec mp2 -ab 64k -vcodec mpeg4"
fi

if [ -n "$do_ogg_vp3" ] ; then
# -idct simple causes different results on different systems
DEC_OPTS="$DEC_OPTS -idct auto"
do_lavf_fate ogg "vp3/coeff_level64.mkv"
fi

if [ -n "$do_wtv" ] ; then
do_lavf wtv "-acodec mp2"
fi


# streamed images
# mjpeg
#file=${outfile}lavf.mjpeg
#do_avconv $file -t 1 -qscale 10 -f image2 -vcodec pgmyuv -i $raw_src
#do_avconv_crc $file -i $target_path/$file

if [ -n "$do_pbmpipe" ] ; then
do_streamed_images pbm
fi

if [ -n "$do_pgmpipe" ] ; then
do_streamed_images pgm
fi

if [ -n "$do_ppmpipe" ] ; then
do_streamed_images ppm
fi

if [ -n "$do_gif" ] ; then
file=${outfile}lavf.gif
do_avconv $file $DEC_OPTS -f image2 -vcodec pgmyuv -i $raw_src $ENC_OPTS -t 1 -qscale 10 -pix_fmt rgb24
do_avconv_crc $file $DEC_OPTS -i $target_path/$file -pix_fmt rgb24
fi

if [ -n "$do_yuv4mpeg" ] ; then
file=${outfile}lavf.y4m
do_avconv $file $DEC_OPTS -f image2 -vcodec pgmyuv -i $raw_src $ENC_OPTS -t 1 -qscale 10
#do_avconv_crc $file -i $target_path/$file
fi

# image formats

if [ -n "$do_pgm" ] ; then
do_image_formats pgm
fi

if [ -n "$do_ppm" ] ; then
do_image_formats ppm
fi

if [ -n "$do_png" ] ; then
do_image_formats png
do_image_formats png "-pix_fmt gray16be"
do_image_formats png "-pix_fmt rgb48be"
fi

if [ -n "$do_bmp" ] ; then
do_image_formats bmp
fi

if [ -n "$do_tga" ] ; then
do_image_formats tga
fi

if [ -n "$do_tiff" ] ; then
do_image_formats tiff "-pix_fmt rgb24"
fi

if [ -n "$do_sgi" ] ; then
do_image_formats sgi
fi

if [ -n "$do_jpg" ] ; then
do_image_formats jpg "-pix_fmt yuvj420p" "-f image2"
fi

if [ -n "$do_pcx" ] ; then
do_image_formats pcx
fi

if [ -n "$do_dpx" ] ; then
do_image_formats dpx
fi

if [ -n "$do_xwd" ] ; then
do_image_formats xwd
fi

if [ -n "$do_sunrast" ] ; then
do_image_formats sun
fi

# audio only

if [ -n "$do_wav" ] ; then
do_audio_only wav
fi

if [ -n "$do_alaw" ] ; then
do_audio_only al "" "" "-ar 44100"
fi

if [ -n "$do_mulaw" ] ; then
do_audio_only ul "" "" "-ar 44100"
fi

if [ -n "$do_au" ] ; then
do_audio_only au
fi

if [ -n "$do_mmf" ] ; then
do_audio_only mmf
fi

if [ -n "$do_aiff" ] ; then
do_audio_only aif
fi

if [ -n "$do_voc" ] ; then
do_audio_only voc
fi

if [ -n "$do_voc_s16" ] ; then
do_audio_only s16.voc "-ac 2" "-acodec pcm_s16le"
fi

if [ -n "$do_ogg" ] ; then
do_audio_only ogg
fi

if [ -n "$do_rso" ] ; then
do_audio_only rso
fi

if [ -n "$do_sox" ] ; then
do_audio_only sox
fi

if [ -n "$do_caf" ] ; then
do_audio_only caf
fi

# pix_fmt conversions

if [ -n "$do_pixfmt" ] ; then
outfile="$datadir/pixfmt/"
mkdir -p "$outfile"
conversions="yuv420p yuv422p yuv444p yuyv422 yuv410p yuv411p yuvj420p \
             yuvj422p yuvj444p rgb24 bgr24 rgb32 rgb565 rgb555 gray monow \
             monob yuv440p yuvj440p"
for pix_fmt in $conversions ; do
    file=${outfile}${pix_fmt}.yuv
    run_avconv $DEC_OPTS -r 1 -f image2 -vcodec pgmyuv -i $raw_src \
               $ENC_OPTS -f rawvideo -t 1 -s 352x288 -pix_fmt $pix_fmt $target_path/$raw_dst
    do_avconv $file $DEC_OPTS -f rawvideo -s 352x288 -pix_fmt $pix_fmt -i $target_path/$raw_dst \
                    $ENC_OPTS -f rawvideo -s 352x288 -pix_fmt yuv444p
done
fi
