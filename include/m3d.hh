// Copyright 2018 Keri Oleg

#pragma once

namespace neat {
namespace m3d {

enum Chunk : uint16_t {
    Invalid = 0,
    Main = 0x4d4d,
    Version = 0x0002,
    Float = 0x10,
    Byte = 0x11,
    ByteGamma = 0x12,
    FloatGamma = 0x13,
    PercentInt = 0x30,
    PercentFloat = 0x31,
    Editor = 0x3d3d,
    ObjectBlock = 0x4000,
    Mesh = 0x4100,
    Vertices = 0x4110,
    FacesDescr = 0x4120,
    FacesMaterial = 0x4130,
    SmoothGroup = 0x4150,
    MappingCoords = 0x4140,
    LocalCoordSystem = 0x4160,
    Light = 0x4600,
    SpotLight = 0x4610,
    Camera = 0x4700,
    MaterialBlock = 0xafff,
    MaterialName = 0xa000,
    AmbientColor = 0xa010,
    DiffuseColor = 0xa020,
    SpecularColor = 0xa030,
    SpecularExponent = 0xa040,
    Shininess = 0xa041,
    TextureMap = 0xa200,
    BumpMap = 0xa230,
    ReflectionMap = 0xa220,
    ReflectionMapFilename = 0xa300,
    ReflectionMapParam = 0xa351,
    KeyframerChunk = 0xb000,
    MeshInfoBlock = 0xb002,
    SpotLightBlock = 0xb007,
    Frames = 0xb008,
    ObjectName = 0xb010,
    ObjectPivotPoint = 0xb013,
    PositionTrack = 0xb020,
    RotationTrack = 0xb021,
    ScaleTrack = 0xb022,
    Hierarchy = 0xb030
};
}
}  // namespace neat
