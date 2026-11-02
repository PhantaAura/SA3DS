#include "pc2004_ground_accel.h"
#include <math.h>

static int finite_vector(SA1PC2004LocalVector v){
    return isfinite(v.forward)&&isfinite(v.normal)&&isfinite(v.side);
}
/* Source multiplies before its named f32 store, not between products. */
static double limit(float coefficient,float multiplier,float normal){
    volatile double first=(double)coefficient*multiplier;
    return first*normal;
}
static int small(float a){return a>-.051f&&a<.051f;}
static float fallback(float a,float f,float v,float jog,int analog){
    return !analog&&v<=jog&&small(a)?0.0f:(float)((double)a+f);
}
int sa1_pc2004_pgetspeed_local(SA1PC2004LocalVector speed,
    SA1PC2004LocalVector add,int analog,float up,uint32_t flags,
    const SA1SteamPlayerParameters *p,SA1PC2004LocalVector *out){
    if(!out)return 0;
    *out=(SA1PC2004LocalVector){0};
    if(!p||!finite_vector(speed)||!finite_vector(add)||!isfinite(up)||
       !isfinite(p->jog_speed)||!isfinite(p->run_speed)||!isfinite(p->air_break))return 0;
    double retained_forward=(double)speed.forward+add.forward;
    SA1PC2004LocalVector next={(float)retained_forward,
        (float)((double)speed.normal+add.normal),(float)((double)speed.side+add.side)};
    float h=(float)((double)p->jog_speed*.87f);
    if(speed.forward>0.0f){
        if(retained_forward<0.0)next.forward=0;
        else if(!analog&&next.forward<h&&up>.7f)next.forward=0;
    }else if(speed.forward<0.0f){
        if(retained_forward>0.0)next.forward=0;
        else if(!analog&&next.forward>-h&&up>.7f)next.forward=0;
    }
    if(speed.side>0.0f){
        if((speed.side<h&&speed.side==next.side)||
           (speed.side>=h&&next.side<h))next.side=0;
    }else if(speed.side<0.0f){
        if((speed.side>-h&&speed.side==next.side)||
           (speed.side<=-h&&next.side>-h))next.side=0;
    }
    double threshold;
    if(flags&0x400000u)threshold=(double)p->jog_speed*.8f;
    else threshold=((double)p->run_speed+p->jog_speed)*.5f;
    if((flags&0x400002u)&&next.forward>=threshold)
        next.forward=(float)((double)next.forward+p->air_break);
    if(!finite_vector(next))return 0;
    *out=next;return 1;
}
int sa1_pc2004_ground_accel_clamp(const SA1PC2004GroundAccelInput *in,
    const SA1SteamPlayerParameters *p,SA1PC2004GroundAccelResult *out){
    if(!out)return 0;
    *out=(SA1PC2004GroundAccelResult){0};
    if(!in||!p||!finite_vector(in->force_after_resistance)||
       !finite_vector(in->speed_after_turn)||!isfinite(in->input_force)||
       !isfinite(in->up)||!isfinite(in->friction_multiplier)||
       !isfinite(in->normal_addend)||!isfinite(p->lim_frict)||
       !isfinite(p->grd_frict)||!isfinite(p->grd_frict_z)||!isfinite(p->jog_speed))return 0;
    const float a=in->force_after_resistance.forward,n=in->force_after_resistance.normal;
    float z=in->force_after_resistance.side;
    const float f=in->input_force,v=in->speed_after_turn.forward;
    const float m=in->friction_multiplier;
    float forward;
    if(in->up<.71f&&v<p->jog_speed&&v>-p->jog_speed&&!in->analog_active){
        forward=(float)((double)a*10.0);z=(float)((double)z*10.0);
    }else if(v==0.0f){
        float k=(float)limit(p->lim_frict,m,n);
        forward=(float)((double)a+f); /*44C9A7 store precedes comparisons. */
        if(!in->analog_active&&((forward<k&&forward>-k)||small(forward)))forward=0;
    }else if(f>=0.0f){
        float k=(float)limit(p->lim_frict,m,n);
        if(v<0.0f&&small(a)){
            double candidate=(double)a+f;
            forward=(float)candidate;
            if(candidate*f<0.0)forward=0;
        }else if(k>0.0f&&f>k){
            double candidate=(double)a+f;
            forward=(float)candidate;
            if(candidate>k){
                out->animation_residual=(float)(candidate-k);
                out->wrote_residual=1;forward=k;
            }
        }else forward=fallback(a,f,v,p->jog_speed,in->analog_active);
    }else if(!in->surface_flag4){
        if(v>0.0f){
            double candidate=(double)a+f;
            forward=candidate*f<0.0?0.0f:(float)candidate;
        }else if(!in->analog_active&&v<=p->jog_speed&&small(a))forward=0;
        else{
            /*44CB34 keeps the limit retained through the F addition. A is
             * intentionally absent from this asymmetric negative branch. */
            double candidate=limit(p->lim_frict,m,n)+f;
            forward=candidate>=0.0?0.0f:(float)candidate;
        }
    }else{
        double retained_b=n<0.0f?limit(p->grd_frict,m,n):0.0;
        float b=(float)retained_b;
        if(n<0.0f&&retained_b>0.0&&-f>b){
            double candidate=(double)a+f;
            forward=(float)candidate;
            if(candidate<0.0&&candidate < -b){
                /* Literal44CA9C subtraction, including the negative branch. */
                out->animation_residual=(float)(candidate-b);
                out->wrote_residual=1;forward=-b;
            }else if(candidate>0.0&&candidate>b){
                out->animation_residual=(float)(candidate-b);
                out->wrote_residual=1;forward=b;
            }
        }else forward=fallback(a,f,v,p->jog_speed,in->analog_active);
    }
    float side=z;
    if(in->speed_after_turn.side==0.0f){
        double k=limit(p->lim_frict,m,n); /* Side zero branch has no f32 K store. */
        if(z<k&&z>-k)side=0;
    }else{
        float b=n<0.0f?(float)limit(p->grd_frict_z,m,n):0.0f;
        if(in->surface_flag4){
            if(z>0.0f){double next=(double)z-b;side=next<0.0?0.0f:(float)next;}
            else if(z<0.0f){double next=(double)z+b;side=next>0.0?0.0f:(float)next;}
        }else{
            float opposing=z>0.0f?-b:z<0.0f?b:0.0f;
            double next=(double)z+opposing;
            side=z!=0.0f&&opposing!=0.0f&&z*next<0.0?0.0f:(float)next;
        }
    }
    out->add_velocity=(SA1PC2004LocalVector){forward,(float)((double)n+in->normal_addend),side};
    if(!finite_vector(out->add_velocity)||!isfinite(out->animation_residual)){
        *out=(SA1PC2004GroundAccelResult){0};return 0;
    }
    return 1;
}

static float noinput_friction(float speed,float acceleration,float normal,
    float axis_friction,float multiplier,float limit_friction){
    if(speed==0){
        float limit=limit_friction*multiplier*normal;
        return acceleration<limit&&-limit<acceleration?0:acceleration;
    }
    float friction=normal<0?axis_friction*multiplier*normal:0;
    if(acceleration>0)friction=-friction;
    else if(acceleration==0)friction=0;
    acceleration+=friction;
    if((speed>0&&speed < -acceleration)||(speed<0&&-acceleration<speed))return -speed;
    return acceleration;
}
int sa1_pc2004_noinput_add_velocity(SA1PC2004LocalVector speed,
    SA1PC2004LocalVector force,const SA1PC2004MotionContext *ctx,
    const SA1SteamPlayerParameters *p,SA1PC2004LocalVector *out){
    if(!out)return 0;
    *out=(SA1PC2004LocalVector){0};
    if(!ctx||!p||ctx->status_0x4000||!isfinite(ctx->acceleration_multiplier)||
       !isfinite(speed.forward)||!isfinite(speed.normal)||!isfinite(speed.side)||
       !isfinite(force.forward)||!isfinite(force.normal)||!isfinite(force.side))return 0;
    SA1PC2004LocalVector add={force.forward+p->air_resist*speed.forward,
        force.normal+p->air_resist_y*speed.normal,
        force.side+p->air_resist_z*speed.side};
    if(ctx->grounded_status){
        add.forward=noinput_friction(speed.forward,add.forward,add.normal,
            p->grd_frict,ctx->acceleration_multiplier,p->lim_frict);
        add.side=noinput_friction(speed.side,add.side,add.normal,
            p->grd_frict_z,ctx->acceleration_multiplier,p->lim_frict);
    }
    *out=add;return 1;
}
