//
//  Shape.cpp
//  render/src/render
//
//  Created by Zach Pomerantz on 12/31/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shape.h"

#include <PerfStat.h>

using namespace render;

Shape::PipelineLib Shape::_pipelineLib;

const Shape::PipelinePointer Shape::_pickPipeline(RenderArgs* args, const Key& key) {
    assert(!_pipelineLib.empty());
    assert(args);
    assert(args->_batch);

    PerformanceTimer perfTimer("Shape::getPipeline");

    const auto& pipelineIterator = _pipelineLib.find(key);
    if (pipelineIterator == _pipelineLib.end()) {
        qDebug() << "Couldn't find a pipeline from ShapeKey ?" << key;
        return PipelinePointer(nullptr);
    }

    PipelinePointer shapePipeline(pipelineIterator->second);
    auto& batch = args->_batch;

    // Setup the one pipeline (to rule them all)
    batch->setPipeline(shapePipeline->pipeline);

    return shapePipeline;
}

void Shape::PipelineLib::addPipeline(Key key, gpu::ShaderPointer& vertexShader, gpu::ShaderPointer& pixelShader) {
    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("skinClusterBuffer"), Slot::SKINNING_GPU));
    slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), Slot::MATERIAL_GPU));
    slotBindings.insert(gpu::Shader::Binding(std::string("diffuseMap"), Slot::DIFFUSE_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), Slot::NORMAL_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("specularMap"), Slot::SPECULAR_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("emissiveMap"), Slot::LIGHTMAP_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), Slot::LIGHT_BUFFER));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), Slot::NORMAL_FITTING_MAP));

    gpu::ShaderPointer program = gpu::Shader::createProgram(vertexShader, pixelShader);
    gpu::Shader::makeProgram(*program, slotBindings);

    auto locations = std::make_shared<Locations>();
    locations->texcoordMatrices = program->getUniforms().findLocation("texcoordMatrices");
    locations->emissiveParams = program->getUniforms().findLocation("emissiveParams");
    locations->normalFittingMapUnit = program->getTextures().findLocation("normalFittingMap");
    locations->diffuseTextureUnit = program->getTextures().findLocation("diffuseMap");
    locations->normalTextureUnit = program->getTextures().findLocation("normalMap");
    locations->specularTextureUnit = program->getTextures().findLocation("specularMap");
    locations->emissiveTextureUnit = program->getTextures().findLocation("emissiveMap");
    locations->skinClusterBufferUnit = program->getBuffers().findLocation("skinClusterBuffer");
    locations->materialBufferUnit = program->getBuffers().findLocation("materialBuffer");
    locations->lightBufferUnit = program->getBuffers().findLocation("lightBuffer");

    auto state = std::make_shared<gpu::State>();

    // Backface on shadow
    if (key.isShadow()) {
        state->setCullMode(gpu::State::CULL_FRONT);
        state->setDepthBias(1.0f);
        state->setDepthBiasSlopeScale(4.0f);
    } else {
        state->setCullMode(gpu::State::CULL_BACK);
    }

    // Z test depends on transparency
    state->setDepthTest(true, !key.isTranslucent(), gpu::LESS_EQUAL);

    // Blend if transparent
    state->setBlendFunction(key.isTranslucent(),
        gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA, // For transparent only, this keep the highlight intensity
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    // Add the brand new pipeline and cache its location in the lib
    auto pipeline = gpu::Pipeline::create(program, state);
    insert(value_type(key, std::make_shared<Pipeline>(pipeline, locations)));

    // Add a wireframe version
    if (!key.isWireFrame()) {
        ShapeKey wireframeKey(ShapeKey::Builder(key).withWireframe());

        auto wireframeState = std::make_shared<gpu::State>(state->getValues());
        wireframeState->setFillMode(gpu::State::FILL_LINE);

        auto wireframePipeline = gpu::Pipeline::create(program, wireframeState);
        insert(value_type(wireframeKey, std::make_shared<Pipeline>(wireframePipeline, locations)));
    }
}
