#pragma once

#include "loop.hpp"
#include "trace.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>

using namespace glm;

namespace pcs {
    std::mutex ungenerated_trace_mtx;
    std::mutex trace_mtx;

    void path_thread(std::vector<Loop> *loops, std::vector<Trace> *ungenerated_traces, std::vector<Trace> *traces) {
        while(true) {
            ungenerated_trace_mtx.lock();
            if(ungenerated_traces->size() == 0) {
                std::cout << "end" << std::endl;
                ungenerated_trace_mtx.unlock();
                return;
            }
            Trace t = ungenerated_traces->back();
            ungenerated_traces->pop_back();
            ungenerated_trace_mtx.unlock();
            
            t.generate_dynamic_path(loops, 1000);

            trace_mtx.lock();
            traces->push_back(t);
            trace_mtx.unlock();
        }
    }

    void compile_em(std::vector<Loop> *loops, std::vector<Trace> *traces) {
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0, 1, 0), 0.3, 4.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0), 0.3, 4.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.4f, 0.0f), glm::vec3(0, 1, 0), 0.3, 4.0, 100));

        float t_major_radius = 1.4;
        float t_minor_radius = 1.0;
        float ring_radius = 0.125;
        float ring_current = 10.0;
        float tf_current = 1.0;

        int inner_ring_cnt = 6;
        int outer_ring_cnt = 16;
        int top_ring_cnt = 12;

        for(int i = 0; i < inner_ring_cnt; i++) {
            vec3 c = vec3(cos(two_pi<float>() * i / inner_ring_cnt), 0, sin(two_pi<float>() * i / inner_ring_cnt)) * (t_major_radius-t_minor_radius);
            loops->push_back(Loop::create_circle(c, c, ring_radius, ring_current, 40));
        }

        for(int i = 0; i < outer_ring_cnt; i++) {
            vec3 c = vec3(cos(two_pi<float>() * i / outer_ring_cnt), 0, sin(two_pi<float>() * i / outer_ring_cnt)) * (t_major_radius+t_minor_radius);
            loops->push_back(Loop::create_circle(c, -c, ring_radius, ring_current, 40));
        }

        for(int i = 0; i < top_ring_cnt; i++) {
            vec3 c = vec3(cos(two_pi<float>() * i / top_ring_cnt), 0, sin(two_pi<float>() * i / top_ring_cnt)) * (t_major_radius) + vec3(0, 1, 0) * t_minor_radius;
            loops->push_back(Loop::create_circle(c, vec3(0, -1, 0), ring_radius, ring_current, 40));
        }

        for(int i = 0; i < top_ring_cnt; i++) {
            vec3 c = vec3(cos(two_pi<float>() * i / top_ring_cnt), 0, sin(two_pi<float>() * i / top_ring_cnt)) * (t_major_radius) - vec3(0, 1, 0) * t_minor_radius;
            loops->push_back(Loop::create_circle(c, vec3(0, 1, 0), ring_radius, ring_current, 40));
        }


        int num_loops = 12;
        for(int i = 0; i < num_loops; i++) {
            loops->push_back(Loop::create_circle(glm::vec3(cos(two_pi<float>() * i / num_loops), 0, -sin(two_pi<float>() * i / num_loops)) * t_major_radius, glm::vec3(sin(two_pi<float>() * i / num_loops), 0, cos(two_pi<float>() * i / num_loops)), t_minor_radius, tf_current, 100));
        }

        loops->push_back(Loop::create_circle(glm::vec3(0, t_minor_radius, 0), glm::vec3(0, t_minor_radius, 0), t_major_radius + t_minor_radius, 0.2, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0, -t_minor_radius, 0), glm::vec3(0, t_minor_radius, 0), t_major_radius + t_minor_radius, 0.2, 100));


        int levels = 6;
        int samples = 16;
        
        std::vector<Trace> ungenerated_traces;

        for(int r = 0; r < levels; r++) {
            for(int i = 0; i < samples; i++) {
                Trace t = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0+1.4, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), true);
                Trace t2 = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0+1.4, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), false);
                // t.generate_path(loops, 0.005, 1000, true);
                // t2.generate_path(loops, 0.005, 1000, false);
                // t.generate_dynamic_path(loops, 1000, true);
                // t2.generate_dynamic_path(loops, 1000, false);
                // traces->push_back(t);
                // traces->push_back(t2);
                ungenerated_traces.push_back(t);
                ungenerated_traces.push_back(t2);
            }
        }

        int conc = std::thread::hardware_concurrency();
        std::cout << "threads: " << conc << std::endl;

        std::thread ts[conc-1];
        for(int i = 0; i < conc - 1; i++) {
            ts[i] = std::thread(path_thread, loops, &ungenerated_traces, traces);
        }
        std::cout << "before main thread path calcs" << std::endl;
        path_thread(loops, &ungenerated_traces, traces);
        std::cout << "after main thread path calcs" << std::endl;
        for(int i = 0; i < conc - 1; i++) {
            ts[i].join();
        }
        std::cout << "after joins" << std::endl;
    }

    void compile_em_bottle(std::vector<Loop> *loops, std::vector<Trace> *traces) {
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0, 0, 1), 0.5, 4.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0, 0, 1), 1.0, 1.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 0, 1), 1.0, 1.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0, 0, 1), 1.0, 1.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0, 0, 1), 0.5, 4.0, 100));

        int levels = 6;
        int samples = 16;
        
        std::vector<Trace> ungenerated_traces;

        for(int r = 0; r < levels; r++) {
            for(int i = 0; i < samples; i++) {
                Trace t = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), true);
                Trace t2 = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), false);
                // t.generate_path(loops, 0.005, 1000, true);
                // t2.generate_path(loops, 0.005, 1000, false);
                // t.generate_dynamic_path(loops, 1000, true);
                // t2.generate_dynamic_path(loops, 1000, false);
                // traces->push_back(t);
                // traces->push_back(t2);
                ungenerated_traces.push_back(t);
                ungenerated_traces.push_back(t2);
            }
        }

        int conc = std::thread::hardware_concurrency();
        std::cout << "threads: " << conc << std::endl;

        std::thread ts[conc-1];
        for(int i = 0; i < conc - 1; i++) {
            ts[i] = std::thread(path_thread, loops, &ungenerated_traces, traces);
        }
        std::cout << "before main thread path calcs" << std::endl;
        path_thread(loops, &ungenerated_traces, traces);
        std::cout << "after main thread path calcs" << std::endl;
        for(int i = 0; i < conc - 1; i++) {
            ts[i].join();
        }
        std::cout << "after joins" << std::endl;
    }


    void compile_em_twist_test(std::vector<Loop> *loops, std::vector<Trace> *traces) {
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 0, 1), 1.0, 1.0, 100));

        int levels = 6;
        int samples = 16;
        
        std::vector<Trace> ungenerated_traces;

        for(int r = 0; r < levels; r++) {
            for(int i = 0; i < samples; i++) {
                Trace t = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), true);
                Trace t2 = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), false);
                ungenerated_traces.push_back(t);
                ungenerated_traces.push_back(t2);
            }
        }

        int conc = std::thread::hardware_concurrency();
        std::cout << "threads: " << conc << std::endl;

        std::thread ts[conc-1];
        for(int i = 0; i < conc - 1; i++) {
            ts[i] = std::thread(path_thread, loops, &ungenerated_traces, traces);
        }
        std::cout << "before main thread path calcs" << std::endl;
        path_thread(loops, &ungenerated_traces, traces);
        std::cout << "after main thread path calcs" << std::endl;
        for(int i = 0; i < conc - 1; i++) {
            ts[i].join();
        }
        std::cout << "after joins" << std::endl;
    }
};
