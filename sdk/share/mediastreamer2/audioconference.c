﻿/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2011 Belledonne Communications SARL
Author: Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "mediastreamer2/msconference.h"
#include "mediastreamer2/msaudiomixer.h"
#include "private.h"

struct _MSAudioConference{
    MSTicker *ticker;
    MSFilter *mixer;
    MSAudioConferenceParams params;
    int nmembers;
};

struct _MSAudioEndpoint{
    AudioStream *st;
    MSFilter *in_resampler,*out_resampler;
    MSCPoint out_cut_point;
    MSCPoint in_cut_point;
    MSCPoint in_cut_point_prev;
    MSCPoint mixer_in;
    MSCPoint mixer_out;
    MSAudioConference *conference;
    int pin;
    int samplerate;
};

/*to count conference member*/
static uint8_t member_cnt = 0;

MSAudioConference * ms_audio_conference_new(const MSAudioConferenceParams *params){
    MSAudioConference *obj=ms_new0(MSAudioConference,1);
    int tmp=1;
    obj->ticker=ms_ticker_new();
    ms_ticker_set_name(obj->ticker,"Audio conference MSTicker");
    ms_ticker_set_priority(obj->ticker,__ms_get_default_prio(FALSE));
    obj->mixer=ms_filter_new(MS_AUDIO_MIXER_ID);
    obj->params=*params;
    ms_filter_call_method(obj->mixer,MS_AUDIO_MIXER_ENABLE_CONFERENCE_MODE,&tmp);
    ms_filter_call_method(obj->mixer,MS_FILTER_SET_SAMPLE_RATE,&obj->params.samplerate);
	member_cnt = 0;
    return obj;
}

const MSAudioConferenceParams *ms_audio_conference_get_params(MSAudioConference *obj){
    return &obj->params;
}

static MSCPoint just_before(MSFilter *f){
    MSQueue *q;
    MSCPoint pnull={0};
    if ((q=f->inputs[0])!=NULL){
        return q->prev;
    }
    ms_fatal("No filter before %s",f->desc->name);
    return pnull;
}

static MSCPoint just_after(MSFilter *f){
    MSQueue *q;
    MSCPoint pnull={0};
    if ((q=f->outputs[0])!=NULL){
        return q->next;
    }
    ms_fatal("No filter after %s",f->desc->name);
    return pnull;
}

static void cut_audio_stream_graph(MSAudioEndpoint *ep, bool_t is_remote){
    AudioStream *st=ep->st;

    /*stop the audio graph*/
    ms_ticker_detach(st->ms.sessions.ticker,st->soundread);
    if (st->soundwrite) ms_ticker_detach(st->ms.sessions.ticker,st->soundwrite);

    ep->in_cut_point_prev.pin=0;
    //if (is_remote){
        /*we would like to keep the volrecv (MSVolume filter) in the graph to measure the output level*/
    //    ep->in_cut_point_prev.filter=st->volrecv;
    //}else{
        ep->in_cut_point_prev.filter=st->ms.decoder;
    //}
    if (!is_remote){
	    ep->in_cut_point=just_after(ep->in_cut_point_prev.filter);
    	ms_filter_unlink(ep->in_cut_point_prev.filter,ep->in_cut_point_prev.pin,ep->in_cut_point.filter, ep->in_cut_point.pin);
    }
    ep->out_cut_point=just_before(st->ms.encoder);
    ms_filter_unlink(ep->out_cut_point.filter,ep->out_cut_point.pin,st->ms.encoder,0);
	
    ms_filter_call_method(st->ms.rtpsend,MS_FILTER_GET_SAMPLE_RATE,&ep->samplerate);

    if (is_remote){
        ep->mixer_in.filter=ep->in_cut_point_prev.filter;
        ep->mixer_in.pin=ep->in_cut_point_prev.pin;
        ep->mixer_out.filter=st->ms.encoder;
        ep->mixer_out.pin=0;
    }else{
        ep->mixer_in=ep->out_cut_point;
        ep->mixer_out=ep->in_cut_point;
    }
}


static void redo_audio_stream_graph(MSAudioEndpoint *ep){
    AudioStream *st=ep->st;
	if(ep->in_cut_point.filter)
	    ms_filter_link(ep->in_cut_point_prev.filter,ep->in_cut_point_prev.pin,ep->in_cut_point.filter,ep->in_cut_point.pin);
    ms_filter_link(ep->out_cut_point.filter,ep->out_cut_point.pin,st->ms.encoder,0);
    ms_ticker_attach(st->ms.sessions.ticker,st->soundread);
    if (st->soundwrite)
        ms_ticker_attach(st->ms.sessions.ticker,st->soundwrite);
}

static int find_free_pin(MSFilter *mixer){
    int i;
    for(i=0;i<mixer->desc->ninputs;++i){
        if (mixer->inputs[i]==NULL){
            return i;
        }
    }
    ms_fatal("No more free pin in mixer filter");
    return -1;
}

static void plumb_to_conf(MSAudioEndpoint *ep){
    MSAudioConference *conf=ep->conference;
    int in_rate=ep->samplerate,out_rate=ep->samplerate;

    ep->pin=find_free_pin(conf->mixer);

    if (ep->mixer_in.filter){
        ms_filter_link(ep->mixer_in.filter,ep->mixer_in.pin,ep->in_resampler,0);
        ms_filter_link(ep->in_resampler,0,conf->mixer,ep->pin);
    }
    if (ep->mixer_out.filter){
        ms_filter_link(conf->mixer,ep->pin,ep->out_resampler,0);
        ms_filter_link(ep->out_resampler,0,ep->mixer_out.filter,ep->mixer_out.pin);
    }
    /*configure resamplers*/
    ms_filter_call_method(ep->in_resampler,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&conf->params.samplerate);
    ms_filter_call_method(ep->out_resampler,MS_FILTER_SET_SAMPLE_RATE,&conf->params.samplerate);
    ms_filter_call_method(ep->in_resampler,MS_FILTER_SET_SAMPLE_RATE,&in_rate);
    ms_filter_call_method(ep->out_resampler,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&out_rate);

}

void ms_audio_conference_update_member_count(bool_t add){
	if(add)
		member_cnt++;
	else
		member_cnt--;
}

bool_t ms_audio_conference_check_member_ready(MSAudioConference *obj){
	if(obj->nmembers == member_cnt)
		return TRUE;
	else
		return FALSE;
}

void ms_audio_conference_add_member(MSAudioConference *obj, MSAudioEndpoint *ep){
    /* now connect to the mixer */
    ep->conference=obj;
    if (obj->nmembers>0) ms_ticker_detach(obj->ticker,obj->mixer);
    plumb_to_conf(ep);
    ms_ticker_attach(obj->ticker,obj->mixer);
    obj->nmembers++;
}

static void unplumb_from_conf(MSAudioEndpoint *ep){
    MSAudioConference *conf=ep->conference;

    if (ep->mixer_in.filter){
        ms_filter_unlink(ep->mixer_in.filter,ep->mixer_in.pin,ep->in_resampler,0);
        ms_filter_unlink(ep->in_resampler,0,conf->mixer,ep->pin);
    }
    if (ep->mixer_out.filter){
        ms_filter_unlink(conf->mixer,ep->pin,ep->out_resampler,0);
        ms_filter_unlink(ep->out_resampler,0,ep->mixer_out.filter,ep->mixer_out.pin);
    }
}

void ms_audio_conference_remove_member(MSAudioConference *obj, MSAudioEndpoint *ep){
    ms_ticker_detach(obj->ticker,obj->mixer);
    unplumb_from_conf(ep);
    ep->conference=NULL;
    obj->nmembers--;
    if (obj->nmembers>0) ms_ticker_attach(obj->ticker,obj->mixer);
}

void ms_audio_conference_mute_member(MSAudioConference *obj, MSAudioEndpoint *ep, bool_t muted){
    MSAudioMixerCtl ctl={0};
    ctl.pin=ep->pin;
    ctl.param.active=!muted;
    ms_filter_call_method(ep->conference->mixer, MS_AUDIO_MIXER_SET_ACTIVE, &ctl);
}

int ms_audio_conference_get_size(MSAudioConference *obj){
    return obj->nmembers;
}


void ms_audio_conference_destroy(MSAudioConference *obj){
    ms_ticker_destroy(obj->ticker);
    ms_filter_destroy(obj->mixer);
    ms_free(obj);
}

MSAudioEndpoint *ms_audio_endpoint_new(void){
    MSAudioEndpoint *ep=ms_new0(MSAudioEndpoint,1);
    ep->in_resampler=ms_filter_new(MS_RESAMPLE_ID);
    ep->out_resampler=ms_filter_new(MS_RESAMPLE_ID);
    ep->samplerate=8000;
    return ep;
}

MSAudioEndpoint * ms_audio_endpoint_get_from_stream(AudioStream *st, bool_t is_remote){
    MSAudioEndpoint *ep=ms_audio_endpoint_new();
    ep->st=st;
    cut_audio_stream_graph(ep,is_remote);
    return ep;
}

void ms_audio_endpoint_release_from_stream(MSAudioEndpoint *obj){
    redo_audio_stream_graph(obj);
    ms_audio_endpoint_destroy(obj);
}

void ms_audio_endpoint_destroy(MSAudioEndpoint *ep){
    if (ep->in_resampler) ms_filter_destroy(ep->in_resampler);
    if (ep->out_resampler) ms_filter_destroy(ep->out_resampler);
    ms_free(ep);
}

