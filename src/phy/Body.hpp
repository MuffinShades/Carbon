#include <iostream>
#include "../msutil.hpp"
#include "../vec.hpp"

struct MaterialProps {

};

enum class Material {
    __n_materials
};

const MaterialProps RB_Materal_Vals[(size_t) Material::__n_materials] =  {

};

struct Force {
    vec3 pos, F;
};