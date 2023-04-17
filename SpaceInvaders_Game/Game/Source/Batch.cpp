// batch.cpp

#include "batch.hpp"

#include <cassert>
#include <cmath>
#include <numbers>

namespace gravity
{
   namespace
   {
      static constexpr float kPI2 = std::numbers::pi_v<float> * 2.0f;

      inline float length(const sf::Vector2f &rhs)
      {
         return std::sqrtf(rhs.x * rhs.x + rhs.y * rhs.y);
      }

      inline sf::Vector2f normalized(const sf::Vector2f &rhs)
      {
         const float l = 1.0f / length(rhs);
         const float x = rhs.x * l;
         const float y = rhs.y * l;
         return sf::Vector2f{ x, y };
      }

      inline sf::Vector2f perpendicular(const sf::Vector2f &rhs)
      {
         return sf::Vector2f{ rhs.y, -rhs.x };
      }

      inline sf::Vector2f top_left(const sf::FloatRect &rect)
      {
         return sf::Vector2f{ rect.left, rect.top };
      }

      inline sf::Vector2f top_right(const sf::FloatRect &rect)
      {
         return sf::Vector2f{ rect.left + rect.width, rect.top };
      }

      inline sf::Vector2f bottom_left(const sf::FloatRect &rect)
      {
         return sf::Vector2f{ rect.left, rect.top + rect.height };
      }

      inline sf::Vector2f bottom_right(const sf::FloatRect &rect)
      {
         return sf::Vector2f{ rect.left + rect.width, rect.top + rect.height };
      }

      inline void push_triangles(sf::VertexArray &vertex_array, const std::initializer_list<sf::Vertex> &&list)
      {
         assert((list.size() % 3) == 0);
         for (const sf::Vertex &vertex : list) {
            vertex_array.append(vertex);
         }
      }
   } // !anon

   primitive_batch::primitive_batch()
      : m_vertex_capacity(default_vertex_count)
   {
      m_vertex_array.resize(m_vertex_capacity);
      m_vertex_buffer.create(m_vertex_capacity);
      m_vertex_buffer.setPrimitiveType(sf::PrimitiveType::Triangles);
   }

   primitive_batch::primitive_batch(std::size_t triangle_capacity)
      : m_vertex_capacity(triangle_capacity * 3)
   {
      m_vertex_array.resize(m_vertex_capacity);
      m_vertex_buffer.create(m_vertex_capacity);
      m_vertex_buffer.setPrimitiveType(sf::PrimitiveType::Triangles);
   }

   void primitive_batch::clear()
   {
      m_vertex_array.clear();
   }

   void primitive_batch::draw_line(const sf::Vector2f &from, const sf::Vector2f &to, const float thickness, const sf::Color &color)
   {
      draw_line(from, to, thickness, color, color);
   }

   void primitive_batch::draw_line(const sf::Vector2f &from, const sf::Vector2f &to, const float thickness, const sf::Color &from_color, const sf::Color &to_color)
   {
      const sf::Vector2f perp = perpendicular(normalized(to - from));
      const sf::Vector2f disp = perp * thickness * 0.5f;

      const sf::Vector2f p0 = (from + disp);
      const sf::Vector2f p1 = (to + disp);
      const sf::Vector2f p2 = (to - disp);
      const sf::Vector2f p3 = (from - disp);

      const sf::Vertex v0{ p0, from_color };
      const sf::Vertex v1{ p1, to_color };
      const sf::Vertex v2{ p2, to_color };
      const sf::Vertex v3{ p3, from_color };

      push_triangles(m_vertex_array, { v0, v1, v2, v2, v3, v0 });
   }

   void primitive_batch::draw_circle(const sf::Vector2f &center, const float radius, const int sections, const sf::Color &color)
   {
      const sf::Vector2f p0 = center;
      sf::Vector2f p1 = center + sf::Vector2f{ radius, 0.0f };

      for (int i = 1; i <= sections; i++) {
         const float theta = float(i) / float(sections) * kPI2;
         const sf::Vector2f p2{ center.x + std::cosf(theta) * radius, center.y + std::sinf(theta) * radius };

         const sf::Vertex v0 = { p0, color };
         const sf::Vertex v1 = { p1, color };
         const sf::Vertex v2 = { p2, color };
         push_triangles(m_vertex_array, { v0, v1, v2 });

         p1 = p2;
      }
   }

   void primitive_batch::draw_circle(const sf::Vector2f &center, const float radius, const int sections, const float thickness, const sf::Color &color)
   {
      const float radius_outer = radius;
      const float radius_inner = radius - thickness;
      sf::Vector2f p1_outer = center + sf::Vector2f{ radius_outer, 0.0 };
      sf::Vector2f p1_inner = center + sf::Vector2f{ radius_inner, 0.0 };

      for (int i = 1; i <= sections; i++) {
         const float theta = float(i) / float(sections) * kPI2;
         const sf::Vector2f normal{ std::cosf(theta), std::sinf(theta) };

         const sf::Vector2f p2_outer{ center.x + normal.x * radius_outer, center.y + normal.y * radius_outer };
         const sf::Vector2f p2_inner{ center.x + normal.x * radius_inner, center.y + normal.y * radius_inner };

         const sf::Vertex v0 = { p1_inner, color };
         const sf::Vertex v1 = { p1_outer, color };
         const sf::Vertex v2 = { p2_outer, color };
         const sf::Vertex v3 = { p2_inner, color };
         push_triangles(m_vertex_array, { v0, v1, v2, v2, v3, v0 });

         p1_outer = p2_outer;
         p1_inner = p2_inner;
      }
   }

   void primitive_batch::draw_rectangle(const sf::FloatRect &rect, const sf::Color &color)
   {
      const sf::Vector2f p0 = top_left(rect);
      const sf::Vector2f p1 = top_right(rect);
      const sf::Vector2f p2 = bottom_right(rect);
      const sf::Vector2f p3 = bottom_left(rect);

      const sf::Vertex v0{ p0, color };
      const sf::Vertex v1{ p1, color };
      const sf::Vertex v2{ p2, color };
      const sf::Vertex v3{ p3, color };

      push_triangles(m_vertex_array, { v0, v1, v2, v2, v3, v0 });
   }

   void primitive_batch::draw_rectangle(const sf::FloatRect &rect, const float thickness, const sf::Color &color)
   {
      const float pxy = thickness * 0.5f;
      const float px0 = rect.left;
      const float px1 = rect.left + rect.width;
      const float py0 = rect.top;
      const float py1 = rect.top + rect.height;

      draw_line(sf::Vector2f{ px0 - pxy, py0 }, sf::Vector2f{ px1 + pxy, py0 }, thickness, color);
      draw_line(sf::Vector2f{ px1, py0 - pxy }, sf::Vector2f{ px1, py1 + pxy }, thickness, color);
      draw_line(sf::Vector2f{ px1 + pxy, py1 }, sf::Vector2f{ px0 - pxy, py1 }, thickness, color);
      draw_line(sf::Vector2f{ px0, py1 + pxy }, sf::Vector2f{ px0, py0 - pxy }, thickness, color);
   }

   void primitive_batch::present(sf::RenderTarget &target)
   {
      if (m_vertex_array.getVertexCount() == 0) {
         return;
      }

      assert(m_vertex_array.getVertexCount() < m_vertex_capacity);
      m_vertex_buffer.update(&m_vertex_array[0], m_vertex_array.getVertexCount(), 0);
      target.draw(m_vertex_buffer, 0, m_vertex_array.getVertexCount());
   }
} // !runner
