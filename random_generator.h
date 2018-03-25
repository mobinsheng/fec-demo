#ifndef RANDOMGENERATOR_H
#define RANDOMGENERATOR_H
/*
 * 随机数产生器
 */
#include <random>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include <time.h>

using namespace std;

class RandomGenerator{
public:
    enum{
        kNormalDistribution,
        kRandomDistribution ,
    };

    RandomGenerator(){
        this->mode = kNormalDistribution;
        SetNormalDistributionParam(700,300);
        SetRandomDistributionParam(400,1000);
    }

    void SetMode(int mode){
        this->mode = mode;
    }

    void SetNormalDistributionParam(int avg,int mse){
        engine tmp_gen(GetSeed());
        normal_gen = tmp_gen;

        normal_distribution<double> tmp_normal(avg,mse);
        normal_dist = tmp_normal;
    }

    void SetRandomDistributionParam(int low,int up){
        engine tmp_gen(GetSeed());
        random_gen = tmp_gen;

        uniform_int_distribution<int> tmp_random(low,up);
        random_dist = tmp_random;
    }

    int RandomNum(){
        if(mode == kNormalDistribution){
            return normal_dist(normal_gen);
        }
        else{
            return random_dist(random_gen);
        }
    }
    static void Test(int mode,int first,int second){
        RandomGenerator g;

        g.SetMode(mode);
        g.SetNormalDistributionParam(first,second);
        g.SetRandomDistributionParam(first,second);

        if(mode == RandomGenerator::kNormalDistribution){
            std::map<int, int> hist;
            for(int n=0; n<10000; ++n) {
                ++hist[g.RandomNum()];
            }
            for(auto p : hist) {
                std::cout << std::fixed << std::setprecision(1) << std::setw(2)
                          << p.first << ' ' << std::string(p.second/10, '*') << '\n';
            }
        }
        else{
            for(int n=0; n<10; ++n)
                std::cout << g.RandomNum() << ' ';
            std::cout << '\n';
        }
    }

private:
    int64_t GetSeed(){
        return rd();
        //return time(NULL);
    }

  //  typedef std::default_random_engine engine ;
    typedef std::mt19937 engine ;

private:

    random_device rd; // 种子产生器
    engine normal_gen; // 随机数引擎，用于产生随机数
    engine random_gen;
    normal_distribution<double> normal_dist; // 分布模型，让引擎产生的随机数符合某一个分布
    uniform_int_distribution<int> random_dist;
    int mode;
};

#endif // RANDOMGENERATOR_H
