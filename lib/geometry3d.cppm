module;


export module gemoetry3d;

struct cube_type
{
    enum direction_type : int
    {
        no_direction = -1,

        left_right = 0b00,
        bottom_top = 0b01,
        back_front = 0b10,

        total_directions = 3,
    };

    enum corner_type : int
    {
        no_corner = -1,

        back_bottom_left   = 0b000,
        back_bottom_right  = 0b001,
        front_bottom_left  = 0b010,
        front_bottom_right = 0b011,

        back_top_left      = 0b100,
        back_top_right     = 0b101,
        front_top_left     = 0b110,
        front_top_right    = 0b111,

        total_corners = 8,
    };

    class corner_bitset : public std::bitset<total_corners>
    {
        using base_type = std::bitset<total_corners>;

        bool operator[](corner_type corner) const
        {
            if( corner <= no_corner || corner >= total_corners )
                return false;
            
            return test( static_cast<size_t>( corner ) );
        }

        reference operator[](corner_type corner)
        { return base_type::operator[]( static_cast<size_t>( corner )); }

        template<typename BitsType>
        BitsType to() const
        { return to_ullong(); }

        template<class Predicate>
        static corner_bitset from_predicate(Predicate&& pred)
        {
            corner_bitset ret;
            for( int i = 0; i < total_corners; ++i)
            {
                corner_type corner = static_cast<corner_type>( i );
                ret[corner] = pred( corner );
            }
            return ret;
        }

        template<class List>
        static corner_bitset from_list(List&& list)
        {
            corner_bitset ret;
            for( auto const& corner : list )
                ret[corner] = true;
            return ret;
        }

        template<class Array>
        static corner_bitset from_array(Array&& array)
        {
            corner_bitset ret;
            for( int i = 0; i < total_corners; ++i)
            {
                corner_type corner = static_cast<corner_type>( i );
                ret[corner] = array[i];
            }
            return ret;
        }

        corner_bitset& operator=( corner_bitset&& ) = default;
        corner_bitset& operator=( corner_bitset const& ) = default;

        corner_bitset() : base_type{} { }
        corner_bitset( unsigned long long bits ) : base_type{ bits } { }
        corner_bitset( corner_bitset&& ) = default;
        corner_bitset( corner_bitset const& ) = default;
    };

    enum edge_type : int
    {
        no_edge = -1,
        
        /* 0b{direction}{first}{second} */
        back_bottom   = 0b0000,
        front_bottom  = 0b0001,
        back_top      = 0b0010,
        front_top     = 0b0011,

        bottom_left   = 0b0100,
        bottom_right  = 0b0101,
        top_left      = 0b0110,
        top_right     = 0b0111,

        back_left     = 0b1000,
        back_right    = 0b1001,
        front_left    = 0b1010,
        front_right   = 0b1011,

        total_edges = 12,
    };

    static constexpr direction_type direction_of(edge_type edge)
    {
        if( edge == no_edge )
            return no_direction;

        return static_cast<direction_type>( static_cast<int>(edge) >> 2 );
    }

    enum side_type : int
    {
        no_side = -1,

        bottom = 0b000,
        top    = 0b001,
        back   = 0b010,
        front  = 0b011,
        left   = 0b100,
        right  = 0b101,

        total_sides = 6,
    };

    static constexpr std::tuple<corner_type, corner_type> 
    corners_of( edge_type edge )
    {
        using corner_pair_type = std::tuple<corner_type, corner_type>;

        static constexpr corner_pair_type edge_corners[] = 
        {
        /* back_bottom   */ { back_bottom_left, back_bottom_right },
        /* front_bottom  */ { front_bottom_left, front_bottom_right },
        /* back_top      */ { back_top_left, back_top_right },
        /* front_top     */ { front_top_left, front_top_right },
        /* bottom_left   */ { back_bottom_left, front_bottom_left },
        /* bottom_right  */ { back_bottom_right, front_bottom_right },
        /* top_left      */ { back_top_left, front_top_left },
        /* top_right     */ { back_top_right, front_top_right },
        /* back_left     */ { back_bottom_left, back_top_left },
        /* back_right    */ { back_bottom_right, back_top_right },
        /* front_left    */ { front_bottom_left, front_top_left },
        /* front_right   */ { front_bottom_right, front_top_right },
        };

        return edge_corners[ edge ];
    }

    static constexpr side_type _side_from(edge_type edge1, edge_type edge2)
    {
        if( edge1 == no_edge || edge2 == no_edge )
            return no_side;

        if( direction_of( edge1 ) != direction_of( edge2 ))
            return no_side;

        int x = static_cast<int>( edge1 ) ^ static_cast<int>( edge2 );


    }

    static constexpr side_type side_from(edge_type edge1, edge_type edge2)
    {
        static constexpr side_type edges_to_side[][total_edges] =
        {
            /*                    back_bottom front_bottom back_top front_top bottom_left bottom_right top_left top_right back_left back_right front_left front_right */
            /* back_bottom   */ { no_side,    bottom,      back,    no_side,  no_side,    no_side,     no_side, no_side,  no_side,  no_side,   no_side,   no_side,     },
            /* front_bottom  */ { bottom,     no_side,     no_side, front,    no_side,    no_side,     no_side, no_side,  no_side,  no_side,   no_side,   no_side,     },
            /* back_top      */ { back,       no_side,     no_side, top,      no_side,    no_side,     no_side, no_side,  no_side,  no_side,   no_side,   no_side,     },
            /* front_top     */ { no_side,    front,       top,     no_side,  no_side,    no_side,     no_side, no_side,  no_side,  no_side,   no_side,   no_side,     },
            /* bottom_left   */ { no_side,    no_side,     no_side, no_side,  no_side,    bottom,      left,    no_side,  no_side,  no_side,   no_side,   no_side,     },
            /* bottom_right  */ { no_side,    no_side,     no_side, no_side,  bottom,     no_side,     no_side, right,    no_side,  no_side,   no_side,   no_side,     },
            /* top_left      */ { no_side,    no_side,     no_side, no_side,  left,       no_side,     no_side, top,      no_side,  no_side,   no_side,   no_side,     },
            /* top_right     */ { no_side,    no_side,     no_side, no_side,  no_side,    right,       top,     no_side,  no_side,  no_side,   no_side,   no_side,     },
            /* back_left     */ { no_side,    no_side,     no_side, no_side,  no_side,    no_side,     no_side, no_side,  no_side,  back,      left,      no_side,     },
            /* back_right    */ { no_side,    no_side,     no_side, no_side,  no_side,    no_side,     no_side, no_side,  back,     no_side,   no_side,   right,       },
            /* front_left    */ { no_side,    no_side,     no_side, no_side,  no_side,    no_side,     no_side, no_side,  left,     no_side,   no_side,   front,       },
            /* front_right   */ { no_side,    no_side,     no_side, no_side,  no_side,    no_side,     no_side, no_side,  no_side,  right,     front,     no_side,     },
        };

        if( edge1 == no_edge || edge2 == no_edge )
            return no_side;
        
        return edges_to_side[edge1][edge2];
    }

    static constexpr edge_type edge_from(corner_type corner1, corner_type corner2)
    {
        static constexpr edge_type corners_to_edge[][total_corners] =
        {
            /*                         back_bottom_left back_bottom_right front_bottom_left front_bottom_right back_top_left back_top_right front_top_left front_top_right */
            /* back_bottom_left   */ { no_edge,         back_bottom,      bottom_left,      no_edge,           back_left,    no_edge,       no_edge,       no_edge,         },
            /* back_bottom_right  */ { back_bottom,     no_edge,          no_edge,          bottom_right,      no_edge,      back_right,    no_edge,       no_edge,         },
            /* front_bottom_left  */ { bottom_left,     no_edge,          no_edge,          front_bottom,      no_edge,      no_edge,       front_left,    no_edge,         },
            /* front_bottom_right */ { no_edge,         bottom_right,     front_bottom,     no_edge,           no_edge,      no_edge,       no_edge,       front_right,     },
            /* back_top_left      */ { back_left,       no_edge,          no_edge,          no_edge,           no_edge,      back_top,      top_left,      no_edge,         },
            /* back_top_right     */ { no_edge,         back_right,       no_edge,          no_edge,           back_top,     no_edge,       no_edge,       top_right,       },
            /* front_top_left     */ { no_edge,         no_edge,          front_left,       no_edge,           top_left,     no_edge,       no_edge,       front_top,       },
            /* front_top_right    */ { no_edge,         no_edge,          no_edge,          front_right,       no_edge,      top_right,     front_top,     no_edge,         },
        };

        if( corner1 == no_corner || corner2 == no_corner )
            return no_edge;

        return corners_to_edge[corner1][corner2];

    }

    static constexpr edge_type edge_from(side_type side1, side_type side2)
    {
        static constexpr edge_type sides_to_edge[][total_sides] = 
        {
            /*              left         right,        top,       bottom,       back,        front        */
            /* left:   */ { no_edge,     no_edge,      top_left,  bottom_left,  back_left,   front_left   },
            /* right:  */ { no_edge,     no_edge,      top_right, bottom_right, back_right,  front_right  },
            /* top:    */ { top_left,    top_right,    no_edge,   no_edge,      back_top,    front_top    },
            /* bottom: */ { bottom_left, bottom_right, no_edge,   no_edge,      back_bottom, front_bottom },
            /* back:   */ { back_left,   back_right,   back_top,  back_bottom,  no_edge,     no_edge      },
            /* front:  */ { front_left,  front_right,  front_top, front_bottom, no_edge,     no_edge      },
        };

        if (side1 == no_side || side2 == no_side)
            return no_edge;

        return sides_to_edge[side1][side2];
    }


    static constexpr corner_type corner_from(edge_type edge, side_type side)
    {
        static constexpr corner_type edge_side_to_corner[][total_sides] =
        {
            /*                     left               right               bottom              top              back               front              */
            /* back_bottom:   */ { back_bottom_left,  back_bottom_right,  no_corner,          no_corner,       no_corner,         no_corner          },
            /* bottom_right:  */ { no_corner,         no_corner,          no_corner,          no_corner,       back_bottom_right, front_bottom_right },
            /* front_bottom:  */ { front_bottom_left, front_bottom_right, no_corner,          no_corner,       no_corner,         no_corner          },
            /* bottom_left:   */ { no_corner,         no_corner,          no_corner,          no_corner,       back_bottom_left,  front_bottom_right },
            /* back_top:      */ { back_top_left,     back_top_right,     no_corner,          no_corner,       no_corner,         no_corner          },
            /* top_right:     */ { no_corner,         no_corner,          no_corner,          no_corner,       back_top_right,    front_top_right    },
            /* front_top:     */ { front_top_left,    front_top_right,    no_corner,          no_corner,       no_corner,         no_corner          },
            /* top_left:      */ { no_corner,         no_corner,          no_corner,          no_corner,       back_top_left,     front_top_left     },
            /* back_left:     */ { no_corner,         no_corner,          back_bottom_left,   back_top_left,   no_corner,         no_corner          },
            /* back_right:    */ { no_corner,         no_corner,          back_bottom_right,  back_top_right,  no_corner,         no_corner          },
            /* front_right:   */ { no_corner,         no_corner,          front_bottom_right, front_top_right, no_corner,         no_corner          },
            /* front_left:    */ { no_corner,         no_corner,          front_bottom_left,  front_top_left,  no_corner,         no_corner          },
        };

        if(edge == no_edge || side == no_side)
            return no_corner;

        return edge_side_to_corner[edge][side];
    }

    static constexpr corner_type corner_from(side_type side1, side_type side2, side_type side3)
    { return corner_from( edge_from( side1, side2 ), side3 ); }

    /* TODO: create the table of triangle patterns and their intersections */
    
    template<typename RelativeField>
    struct triangle_interpolator : 
        public std::ranges::range_adaptor_closure< 
            triangle_interpolator< RelativeField >>
    {
        triangle_view operator()(triangle_patt)
    };

    template<typename FloatType>
    struct isometric_cell
    {
        static constexpr int max_triangles = 5;
        using value_type = FloatType;

        template<class RelativeField>
        isometric_cell(RelativeField&& field)
        { isometric( std::move( field )); }

        template<class RelativeField>
        void isometric(RelativeField&& field, value_type value = 0.)
        {
            auto of_field = std::bind(field_accessor, this);

            auto corners_in = corners() | of_field(field) | greater_than(value);
        
            create_triangles_from( corners_in, field );
        }

    protected:
        template<class RelativeField>
        void create_triangles_from(auto const& corners_in, 
            RelativeField const& field, value_type value)
        {
            m_corners = corners_in | to_bitfield;

            auto triangle_pattern = pattern_from( m_corners );

            m_triangles = triangle_pattern | interpolate_from( field, value );
            m_triangle_count = triangle_pattern.count();
        }

        template<class RelativeField>
        corner_view_of< value_type > field_accessor(RelativeField&& field);

        using triangle_type = std::array<edge_type, 3>;

        static pattern_type pattern_from( corner_bitset const& corners )
        {

        }

    private:
        corner_bitset m_corners;
        int m_triangle_count;
        std::array<triangle_type, max_triangles> m_triangles;
    };

    constexpr void tetrahedral_march(auto const& is_inside)
    {
        // check all the sides
        
        // check all the edges
        // check all the corners
    }


    
};
