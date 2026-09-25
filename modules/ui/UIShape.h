#ifndef _EOKAS_UI_SHAPE_H_
#define _EOKAS_UI_SHAPE_H_

#include "header.h"
#include <memory>
#include <vector>

namespace eokas
{
    class UIShape : public Primitive
    {
    public:
        using Ref = std::shared_ptr<UIShape>;
        DynamicBuffer::Ref vertexBuffer;
        uint32_t vertexLength = 0;
        uint32_t vertexStride = sizeof(UIVertex);
        DynamicBuffer::Ref indexBuffer;
        uint32_t indexLength = 0;
        Format indexFormat = Format::R32_UINT;
        Texture::Ref texture;
        uint32_t indexCount = 0;

        UIShape();

        void begin();
        void setTexture(Texture::Ref tex);
        Vector2 offset() const { return mOffset; }
        void pushOffset(const Vector2& next);
        void popOffset();
        void pushClip(const Rect& screenClip);
        void popClip();
        bool outsideClip(const Rect& local) const;
        void addQuad(const Rect& screen, const Rect& uv, const Color& color);
        void addQuad(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Rect& uv, const Color& color);
        void end();
        void setPendingUpload(const std::vector<uint8_t>& rgba, uint32_t atlasSize = 0);

    private:
        void createResources(Device::Ref device) override;
        void upload(CommandBuffer::Ref cmd) override;
        void encode(CommandBuffer::Ref cmd) override;

        std::vector<UIVertex> mVertices;
        std::vector<uint32_t> mIndices;
        std::vector<Vector2> mOffsetStack;
        std::vector<Rect> mClipStack;
        Vector2 mOffset { 0.0f, 0.0f };
        std::vector<uint8_t> mPendingUploadRgba;
        uint32_t mMaxQuads = 2048;
        uint32_t mPendingAtlasSize = 0;
        bool mTextureDirty = false;
    };
}

#endif//_EOKAS_UI_SHAPE_H_
