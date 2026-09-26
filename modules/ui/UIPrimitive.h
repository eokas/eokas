#ifndef _EOKAS_UI_PRIMITIVE_H_
#define _EOKAS_UI_PRIMITIVE_H_

#include "header.h"
#include <memory>
#include <vector>

namespace eokas
{
    class UIPrimitive : public Primitive
    {
    public:
        using Ref = std::shared_ptr<UIPrimitive>;
        DynamicBuffer::Ref vertexBuffer;
        uint32_t vertexLength = 0;
        uint32_t vertexStride = sizeof(UIVertex);
        DynamicBuffer::Ref indexBuffer;
        uint32_t indexLength = 0;
        Format indexFormat = Format::R32_UINT;
        Texture::Ref texture;
        uint32_t indexCount = 0;

        UIPrimitive();

        void begin();
        void setTexture(Texture::Ref tex);
        Vector2 origin() const { return mOrigin; }
        Vector2 scale() const { return mScale; }
        Vector2 toScreen(const Vector2& local) const;
        Rect toScreen(const Rect& local) const;
        void pushTransform(const Vector2& origin, const Vector2& scale);
        void pushScaleAround(const Vector2& pivot, const Vector2& localScale);
        void pushOrigin(const Vector2& next);
        void popOrigin();
        void pushClip(const Rect& screenClip);
        void popClip();
        bool outsideClip(const Rect& local) const;
        void addQuad(const Rect& screen, const Rect& uv, const Color& color);
        void addQuad(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Rect& uv, const Color& color);
        void addTriangles(const Vector2* vertices, uint32_t triangleCount, const Rect& uv, const Color& color);
        void end();
        void setPendingUpload(const std::vector<uint8_t>& rgba, uint32_t atlasSize = 0);

    private:
        void createResources(Device::Ref device) override;
        void upload(CommandBuffer::Ref cmd) override;
        void encode(CommandBuffer::Ref cmd) override;

        struct TransformState
        {
            Vector2 origin;
            Vector2 scale;
        };

        std::vector<UIVertex> mVertices;
        std::vector<uint32_t> mIndices;
        std::vector<TransformState> mTransformStack;
        std::vector<Rect> mClipStack;
        Vector2 mOrigin { 0.0f, 0.0f };
        Vector2 mScale { 1.0f, 1.0f };
        std::vector<uint8_t> mPendingUploadRgba;
        uint32_t mMaxQuads = 2048;
        uint32_t mPendingAtlasSize = 0;
        bool mTextureDirty = false;
    };
}

#endif//_EOKAS_UI_PRIMITIVE_H_
