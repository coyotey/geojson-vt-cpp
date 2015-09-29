{
  'includes': [
    'deps/common.gypi',
  ],
  'variables': {
    'gtest%': 0,
    'gtest_static_libs%': [],
    'glfw%': 0,
    'glfw_static_libs%': [],
    'mason_platform': 'osx',
  },
  'targets': [
    { 'target_name': 'geojsonvt',
      'product_name': 'geojsonvt',
      'type': 'static_library',
      'standalone_static_library': 1,

      'include_dirs': [
        'include',
      ],

      'sources': [
        'include/mapbox/geojsonvt/geojson.hpp',
        'include/mapbox/geojsonvt/geojsonvt.hpp',
        'include/mapbox/geojsonvt/geojsonvt_clip.hpp',
        'include/mapbox/geojsonvt/geojsonvt_convert.hpp',
        'include/mapbox/geojsonvt/geojsonvt_simplify.hpp',
        'include/mapbox/geojsonvt/geojsonvt_tile.hpp',
        'include/mapbox/geojsonvt/geojsonvt_types.hpp',
        'include/mapbox/geojsonvt/geojsonvt_util.hpp',
        'include/mapbox/geojsonvt/geojsonvt_wrap.hpp',
        'src/geojson.cpp',
        'src/geojsonvt.cpp',
        'src/geojsonvt_clip.cpp',
        'src/geojsonvt_convert.cpp',
        'src/geojsonvt_simplify.cpp',
        'src/geojsonvt_tile.cpp',
        'src/geojsonvt_wrap.cpp',
      ],

      'variables': {
        'cflags_cc': [
          '<@(variant_cflags)',
          '<@(rapidjson_cflags)',
        ],
        'ldflags': [],
        'libraries': [],
      },

      'conditions': [
        ['OS == "mac"', {
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
          },
        }, {
          'cflags_cc': [ '<@(cflags_cc)' ],
        }],
      ],

      'link_settings': {
        'conditions': [
          ['OS == "mac"', {
            'libraries': [ '<@(libraries)' ],
            'xcode_settings': { 'OTHER_LDFLAGS': [ '<@(ldflags)' ] }
          }, {
            'libraries': [ '<@(libraries)', '<@(ldflags)' ],
          }]
        ],
      },

      'direct_dependent_settings': {
        'include_dirs': [
          'include',
        ],
      },
    },

    { 'target_name': 'install',
      'type': 'none',
      'hard_dependency': 1,
      'dependencies': [
        'geojsonvt',
      ],

      'copies': [
        { 'files': [ '<(PRODUCT_DIR)/libgeojsonvt.a' ], 'destination': '<(install_prefix)/lib' },
        { 'files': [ '<!@(find include -name "*.hpp")' ], 'destination': '<(install_prefix)/include/mapbox/geojsonvt' },
      ],
    },
  ],

  'conditions': [
    ['gtest', {
      'targets': [
        { 'target_name': 'test',
          'product_name': 'test',
          'type': 'executable',

          'dependencies': [
            'geojsonvt',
          ],

          'include_dirs': [
            'src',
          ],

          'sources': [
            'test/test.cpp',
            'test/util.hpp',
            'test/util.cpp',
            'test/test_clip.cpp',
            'test/test_full.cpp',
            'test/test_simplify.cpp',
            'test/test_geojson.cpp',
          ],

          'variables': {
            'cflags_cc': [
              '<@(rapidjson_cflags)',
              '<@(variant_cflags)',
              '<@(gtest_cflags)',
            ],
            'ldflags': [
              '<@(gtest_ldflags)'
            ],
            'libraries': [
              '<@(gtest_static_libs)',
            ],
          },

          'conditions': [
            ['OS == "mac"', {
              'libraries': [ '<@(libraries)' ],
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
                'OTHER_LDFLAGS': [ '<@(ldflags)' ],
              }
            }, {
              'cflags_cc': [ '<@(cflags_cc)' ],
              'libraries': [ '<@(libraries)', '<@(ldflags)' ],
            }]
          ],
        },
      ],
    }],
    ['glfw', {
      'targets': [
        { 'target_name': 'debug',
          'product_name': 'debug',
          'type': 'executable',

          'dependencies': [
            'geojsonvt',
          ],

          'include_dirs': [
            'src',
          ],

          'sources': [
            'debug/debug.cpp',
          ],

          'variables': {
            'cflags_cc': [
              '<@(variant_cflags)',
              '<@(glfw_cflags)',
            ],
            'ldflags': [
              '<@(glfw_ldflags)'
            ],
            'libraries': [
              '<@(glfw_static_libs)',
            ],
          },

          'conditions': [
            ['OS == "mac"', {
              'libraries': [ '<@(libraries)' ],
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
                'OTHER_LDFLAGS': [ '<@(ldflags)' ],
              }
            }, {
              'cflags_cc': [ '<@(cflags_cc)' ],
              'libraries': [ '<@(libraries)', '<@(ldflags)' ],
            }]
          ],
        },
      ],
    }],
  ],
}
