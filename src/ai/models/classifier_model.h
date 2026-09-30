


    // !!! This file is generated using emlearn !!!

    #include <stdint.h>
    

static inline int32_t classifier_model_tree_0(const float *features, int32_t features_length) {
          if (features[6] < -0.003624f) {
              if (features[4] < 0.386246f) {
                  return 5;
              } else {
                  return 3;
              }
          } else {
              if (features[1] < 0.322023f) {
                  if (features[7] < 0.071120f) {
                      return 1;
                  } else {
                      if (features[1] < 0.149779f) {
                          if (features[1] < 0.145151f) {
                              if (features[5] < 0.544580f) {
                                  return 0;
                              } else {
                                  if (features[6] < 0.000184f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 0.543579f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[1] < 0.157332f) {
                              if (features[0] < 0.525275f) {
                                  if (features[7] < 0.189292f) {
                                      return 2;
                                  } else {
                                      return 3;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[4] < 0.749161f) {
                                  if (features[5] < 0.271914f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 0;
                              }
                          }
                      }
                  }
              } else {
                  return 4;
              }
          }
        }
        

static inline int32_t classifier_model_tree_1(const float *features, int32_t features_length) {
          if (features[7] < 0.075621f) {
              if (features[7] < 0.067141f) {
                  if (features[6] < 0.002209f) {
                      return 5;
                  } else {
                      return 1;
                  }
              } else {
                  return 4;
              }
          } else {
              if (features[6] < 0.000310f) {
                  if (features[1] < 0.150256f) {
                      return 0;
                  } else {
                      if (features[0] < 0.691939f) {
                          if (features[1] < 0.160243f) {
                              if (features[4] < 0.433235f) {
                                  return 2;
                              } else {
                                  return 3;
                              }
                          } else {
                              if (features[1] < 0.212876f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      } else {
                          return 0;
                      }
                  }
              } else {
                  if (features[5] < 0.894326f) {
                      if (features[4] < 0.485934f) {
                          return 2;
                      } else {
                          if (features[6] < 0.002874f) {
                              return 2;
                          } else {
                              return 3;
                          }
                      }
                  } else {
                      return 0;
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_2(const float *features, int32_t features_length) {
          if (features[6] < 0.006872f) {
              if (features[5] < 0.840192f) {
                  if (features[6] < 0.000276f) {
                      if (features[7] < 0.146271f) {
                          return 0;
                      } else {
                          if (features[4] < 0.433235f) {
                              if (features[7] < 0.188282f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          } else {
                              return 3;
                          }
                      }
                  } else {
                      if (features[4] < 0.485934f) {
                          return 2;
                      } else {
                          if (features[7] < 0.164528f) {
                              return 2;
                          } else {
                              if (features[6] < 0.002874f) {
                                  return 2;
                              } else {
                                  return 3;
                              }
                          }
                      }
                  }
              } else {
                  if (features[1] < 0.346791f) {
                      if (features[1] < 0.293808f) {
                          if (features[6] < 0.000605f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          return 5;
                      }
                  } else {
                      return 4;
                  }
              }
          } else {
              return 1;
          }
        }
        

static inline int32_t classifier_model_tree_3(const float *features, int32_t features_length) {
          if (features[6] < 0.006843f) {
              if (features[4] < 0.185263f) {
                  if (features[0] < 0.424111f) {
                      if (features[4] < 0.105577f) {
                          return 0;
                      } else {
                          return 2;
                      }
                  } else {
                      return 4;
                  }
              } else {
                  if (features[1] < 0.294599f) {
                      if (features[7] < 0.197325f) {
                          if (features[4] < 0.479386f) {
                              if (features[0] < 0.348154f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[1] < 0.151609f) {
                                  return 0;
                              } else {
                                  if (features[1] < 0.193739f) {
                                      if (features[5] < 0.827898f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[1] < 0.162989f) {
                              return 3;
                          } else {
                              if (features[5] < 0.878642f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      }
                  } else {
                      return 5;
                  }
              }
          } else {
              return 1;
          }
        }
        

static inline int32_t classifier_model_tree_4(const float *features, int32_t features_length) {
          if (features[6] < 0.006806f) {
              if (features[1] < 0.301083f) {
                  if (features[6] < 0.000443f) {
                      if (features[6] < -0.004038f) {
                          return 3;
                      } else {
                          if (features[4] < 0.502715f) {
                              if (features[6] < -0.000218f) {
                                  if (features[1] < 0.152015f) {
                                      return 0;
                                  } else {
                                      if (features[7] < 0.190826f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              } else {
                                  if (features[1] < 0.148860f) {
                                      if (features[1] < 0.142910f) {
                                          return 0;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      if (features[1] < 0.160032f) {
                                          return 3;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              return 0;
                          }
                      }
                  } else {
                      if (features[5] < 0.509057f) {
                          if (features[6] < 0.002896f) {
                              return 2;
                          } else {
                              return 3;
                          }
                      } else {
                          return 2;
                      }
                  }
              } else {
                  if (features[7] < 0.056513f) {
                      return 5;
                  } else {
                      return 4;
                  }
              }
          } else {
              return 1;
          }
        }
        

static inline int32_t classifier_model_tree_5(const float *features, int32_t features_length) {
          if (features[7] < 0.042857f) {
              return 5;
          } else {
              if (features[6] < 0.006781f) {
                  if (features[5] < 0.840709f) {
                      if (features[4] < 0.475545f) {
                          if (features[1] < 0.253901f) {
                              return 2;
                          } else {
                              if (features[4] < 0.176133f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[5] < 0.508397f) {
                              if (features[4] < 0.493599f) {
                                  if (features[7] < 0.164897f) {
                                      return 0;
                                  } else {
                                      return 3;
                                  }
                              } else {
                                  return 3;
                              }
                          } else {
                              if (features[6] < 0.000739f) {
                                  if (features[7] < 0.181070f) {
                                      return 0;
                                  } else {
                                      return 3;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  } else {
                      if (features[1] < 0.309203f) {
                          if (features[6] < 0.000605f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          return 4;
                      }
                  }
              } else {
                  return 1;
              }
          }
        }
        

static inline int32_t classifier_model_tree_6(const float *features, int32_t features_length) {
          if (features[0] < 0.546081f) {
              if (features[6] < 0.003186f) {
                  if (features[4] < 0.469025f) {
                      if (features[5] < 0.265132f) {
                          return 0;
                      } else {
                          return 2;
                      }
                  } else {
                      if (features[5] < 0.504536f) {
                          if (features[0] < 0.513507f) {
                              if (features[1] < 0.150529f) {
                                  return 0;
                              } else {
                                  return 3;
                              }
                          } else {
                              return 3;
                          }
                      } else {
                          if (features[6] < -0.002591f) {
                              return 3;
                          } else {
                              if (features[1] < 0.150847f) {
                                  return 0;
                              } else {
                                  return 3;
                              }
                          }
                      }
                  }
              } else {
                  if (features[6] < 0.004479f) {
                      return 4;
                  } else {
                      if (features[0] < 0.494686f) {
                          if (features[4] < 0.389091f) {
                              return 1;
                          } else {
                              return 3;
                          }
                      } else {
                          return 1;
                      }
                  }
              }
          } else {
              if (features[4] < 0.319923f) {
                  return 5;
              } else {
                  if (features[4] < 0.616330f) {
                      return 2;
                  } else {
                      return 0;
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_7(const float *features, int32_t features_length) {
          if (features[0] < 0.546081f) {
              if (features[6] < 0.006806f) {
                  if (features[6] < 0.002805f) {
                      if (features[6] < 0.000259f) {
                          if (features[1] < 0.150123f) {
                              return 0;
                          } else {
                              if (features[5] < 0.289508f) {
                                  return 0;
                              } else {
                                  if (features[4] < 0.434222f) {
                                      return 2;
                                  } else {
                                      return 3;
                                  }
                              }
                          }
                      } else {
                          return 2;
                      }
                  } else {
                      if (features[0] < 0.511409f) {
                          return 3;
                      } else {
                          return 4;
                      }
                  }
              } else {
                  return 1;
              }
          } else {
              if (features[4] < 0.319923f) {
                  return 5;
              } else {
                  if (features[6] < 0.000452f) {
                      return 0;
                  } else {
                      return 2;
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_8(const float *features, int32_t features_length) {
          if (features[6] < 0.006835f) {
              if (features[1] < 0.301083f) {
                  if (features[6] < 0.000267f) {
                      if (features[1] < 0.150910f) {
                          return 0;
                      } else {
                          if (features[4] < 0.709432f) {
                              if (features[5] < 0.281941f) {
                                  return 0;
                              } else {
                                  if (features[4] < 0.433054f) {
                                      return 2;
                                  } else {
                                      return 3;
                                  }
                              }
                          } else {
                              return 0;
                          }
                      }
                  } else {
                      if (features[4] < 0.485934f) {
                          return 2;
                      } else {
                          if (features[0] < 0.576213f) {
                              return 3;
                          } else {
                              if (features[7] < 0.214947f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      }
                  }
              } else {
                  if (features[7] < 0.056513f) {
                      return 5;
                  } else {
                      return 4;
                  }
              }
          } else {
              return 1;
          }
        }
        

static inline int32_t classifier_model_tree_9(const float *features, int32_t features_length) {
          if (features[4] < 0.177156f) {
              if (features[4] < 0.169601f) {
                  if (features[7] < 0.147749f) {
                      return 2;
                  } else {
                      return 0;
                  }
              } else {
                  return 4;
              }
          } else {
              if (features[6] < 0.006806f) {
                  if (features[6] < -0.003539f) {
                      if (features[5] < 0.676376f) {
                          return 3;
                      } else {
                          return 5;
                      }
                  } else {
                      if (features[5] < 0.521757f) {
                          if (features[7] < 0.160683f) {
                              if (features[6] < 0.000464f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 3;
                          }
                      } else {
                          if (features[7] < 0.215451f) {
                              if (features[7] < 0.188011f) {
                                  if (features[4] < 0.751651f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[5] < 0.888626f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              }
                          } else {
                              return 0;
                          }
                      }
                  }
              } else {
                  return 1;
              }
          }
        }
        

static inline int32_t classifier_model_tree_10(const float *features, int32_t features_length) {
          if (features[0] < 0.546081f) {
              if (features[1] < 0.322023f) {
                  if (features[7] < 0.071112f) {
                      return 1;
                  } else {
                      if (features[1] < 0.149039f) {
                          if (features[6] < 0.000321f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[4] < 0.470369f) {
                              if (features[5] < 0.263039f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 3;
                          }
                      }
                  }
              } else {
                  return 4;
              }
          } else {
              if (features[6] < -0.002063f) {
                  return 5;
              } else {
                  if (features[4] < 0.616330f) {
                      return 2;
                  } else {
                      return 0;
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_11(const float *features, int32_t features_length) {
          if (features[1] < 0.346666f) {
              if (features[7] < 0.043865f) {
                  return 5;
              } else {
                  if (features[7] < 0.071428f) {
                      return 1;
                  } else {
                      if (features[7] < 0.197325f) {
                          if (features[6] < 0.000255f) {
                              if (features[6] < -0.001755f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[7] < 0.226213f) {
                              if (features[1] < 0.169524f) {
                                  return 3;
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[1] < 0.208672f) {
                                  return 3;
                              } else {
                                  return 0;
                              }
                          }
                      }
                  }
              }
          } else {
              return 4;
          }
        }
        

static inline int32_t classifier_model_tree_12(const float *features, int32_t features_length) {
          if (features[0] < 0.543582f) {
              if (features[5] < 0.814127f) {
                  if (features[0] < 0.479911f) {
                      if (features[1] < 0.248950f) {
                          if (features[5] < 0.403575f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[4] < 0.124364f) {
                              return 0;
                          } else {
                              return 1;
                          }
                      }
                  } else {
                      if (features[7] < 0.194244f) {
                          if (features[6] < 0.000259f) {
                              return 0;
                          } else {
                              if (features[7] < 0.075502f) {
                                  return 1;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[5] < 0.554356f) {
                              return 3;
                          } else {
                              return 2;
                          }
                      }
                  }
              } else {
                  return 4;
              }
          } else {
              if (features[0] < 0.606621f) {
                  if (features[6] < -0.001689f) {
                      return 5;
                  } else {
                      return 2;
                  }
              } else {
                  if (features[0] < 0.765209f) {
                      if (features[6] < 0.000373f) {
                          return 0;
                      } else {
                          return 2;
                      }
                  } else {
                      return 0;
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_13(const float *features, int32_t features_length) {
          if (features[6] < -0.003513f) {
              if (features[0] < 0.531996f) {
                  return 3;
              } else {
                  return 5;
              }
          } else {
              if (features[1] < 0.322023f) {
                  if (features[4] < 0.479386f) {
                      if (features[4] < 0.295446f) {
                          if (features[5] < 0.652287f) {
                              if (features[0] < 0.231341f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 1;
                          }
                      } else {
                          if (features[6] < 0.007556f) {
                              return 2;
                          } else {
                              return 1;
                          }
                      }
                  } else {
                      if (features[7] < 0.169705f) {
                          if (features[1] < 0.178846f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[0] < 0.692356f) {
                              return 3;
                          } else {
                              return 0;
                          }
                      }
                  }
              } else {
                  return 4;
              }
          }
        }
        

static inline int32_t classifier_model_tree_14(const float *features, int32_t features_length) {
          if (features[5] < 0.842148f) {
              if (features[7] < 0.071120f) {
                  return 1;
              } else {
                  if (features[4] < 0.477731f) {
                      if (features[5] < 0.393296f) {
                          return 0;
                      } else {
                          return 2;
                      }
                  } else {
                      if (features[1] < 0.150723f) {
                          return 0;
                      } else {
                          if (features[4] < 0.511486f) {
                              if (features[5] < 0.661876f) {
                                  return 3;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      }
                  }
              }
          } else {
              if (features[0] < 0.529836f) {
                  return 4;
              } else {
                  if (features[6] < -0.002063f) {
                      return 5;
                  } else {
                      if (features[7] < 0.165860f) {
                          return 2;
                      } else {
                          return 0;
                      }
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_15(const float *features, int32_t features_length) {
          if (features[6] < -0.003513f) {
              if (features[1] < 0.237521f) {
                  return 3;
              } else {
                  return 5;
              }
          } else {
              if (features[7] < 0.075305f) {
                  if (features[6] < 0.006319f) {
                      return 4;
                  } else {
                      return 1;
                  }
              } else {
                  if (features[6] < 0.000267f) {
                      if (features[7] < 0.194238f) {
                          if (features[0] < 0.472931f) {
                              if (features[0] < 0.405076f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 0;
                          }
                      } else {
                          if (features[7] < 0.224139f) {
                              return 3;
                          } else {
                              if (features[7] < 0.231066f) {
                                  return 0;
                              } else {
                                  if (features[4] < 0.265995f) {
                                      return 0;
                                  } else {
                                      return 3;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[7] < 0.200013f) {
                          return 2;
                      } else {
                          if (features[7] < 0.226570f) {
                              if (features[0] < 0.461644f) {
                                  return 2;
                              } else {
                                  if (features[1] < 0.169069f) {
                                      return 3;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              return 0;
                          }
                      }
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_16(const float *features, int32_t features_length) {
          if (features[5] < 0.842148f) {
              if (features[1] < 0.244693f) {
                  if (features[5] < 0.520250f) {
                      if (features[6] < 0.000478f) {
                          if (features[0] < 0.518953f) {
                              if (features[5] < 0.508433f) {
                                  if (features[1] < 0.151476f) {
                                      return 0;
                                  } else {
                                      return 3;
                                  }
                              } else {
                                  if (features[7] < 0.179465f) {
                                      return 0;
                                  } else {
                                      return 3;
                                  }
                              }
                          } else {
                              return 3;
                          }
                      } else {
                          if (features[1] < 0.173491f) {
                              return 3;
                          } else {
                              return 2;
                          }
                      }
                  } else {
                      if (features[7] < 0.085421f) {
                          if (features[7] < 0.079592f) {
                              return 2;
                          } else {
                              return 0;
                          }
                      } else {
                          return 2;
                      }
                  }
              } else {
                  if (features[7] < 0.116692f) {
                      return 1;
                  } else {
                      if (features[7] < 0.200025f) {
                          return 2;
                      } else {
                          return 0;
                      }
                  }
              }
          } else {
              if (features[7] < 0.056218f) {
                  return 5;
              } else {
                  if (features[6] < 0.002588f) {
                      if (features[6] < 0.000605f) {
                          return 0;
                      } else {
                          return 2;
                      }
                  } else {
                      return 4;
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_17(const float *features, int32_t features_length) {
          if (features[7] < 0.076295f) {
              if (features[1] < 0.346791f) {
                  if (features[7] < 0.043959f) {
                      return 5;
                  } else {
                      return 1;
                  }
              } else {
                  return 4;
              }
          } else {
              if (features[4] < 0.483156f) {
                  if (features[7] < 0.213612f) {
                      if (features[7] < 0.131214f) {
                          if (features[7] < 0.122833f) {
                              return 2;
                          } else {
                              return 0;
                          }
                      } else {
                          return 2;
                      }
                  } else {
                      return 0;
                  }
              } else {
                  if (features[1] < 0.150411f) {
                      return 0;
                  } else {
                      if (features[5] < 0.658676f) {
                          return 3;
                      } else {
                          if (features[5] < 0.904609f) {
                              return 2;
                          } else {
                              return 0;
                          }
                      }
                  }
              }
          }
        }
        

static inline int32_t classifier_model_tree_18(const float *features, int32_t features_length) {
          if (features[1] < 0.346791f) {
              if (features[7] < 0.042857f) {
                  return 5;
              } else {
                  if (features[6] < 0.006765f) {
                      if (features[5] < 0.521757f) {
                          if (features[1] < 0.150731f) {
                              return 0;
                          } else {
                              if (features[7] < 0.193147f) {
                                  if (features[7] < 0.149823f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[1] < 0.209812f) {
                                      return 3;
                                  } else {
                                      return 0;
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 0.000440f) {
                              if (features[0] < 0.592392f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          } else {
                              return 2;
                          }
                      }
                  } else {
                      return 1;
                  }
              }
          } else {
              return 4;
          }
        }
        

static inline int32_t classifier_model_tree_19(const float *features, int32_t features_length) {
          if (features[4] < 0.177156f) {
              if (features[7] < 0.092021f) {
                  return 4;
              } else {
                  if (features[0] < 0.241258f) {
                      return 0;
                  } else {
                      return 2;
                  }
              }
          } else {
              if (features[1] < 0.307782f) {
                  if (features[5] < 0.523951f) {
                      if (features[7] < 0.169685f) {
                          if (features[1] < 0.169438f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          return 3;
                      }
                  } else {
                      if (features[6] < 0.005432f) {
                          if (features[4] < 0.616330f) {
                              return 2;
                          } else {
                              return 0;
                          }
                      } else {
                          return 1;
                      }
                  }
              } else {
                  return 5;
              }
          }
        }
        

int32_t classifier_model_predict(const float *features, int32_t features_length) {

        int32_t votes[6] = {0,};
        int32_t _class = -1;

        _class = classifier_model_tree_0(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_1(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_2(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_3(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_4(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_5(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_6(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_7(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_8(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_9(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_10(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_11(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_12(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_13(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_14(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_15(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_16(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_17(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_18(features, features_length); votes[_class] += 1;
    _class = classifier_model_tree_19(features, features_length); votes[_class] += 1;
    
        int32_t most_voted_class = -1;
        int32_t most_voted_votes = 0;
        for (int32_t i=0; i<6; i++) {

            if (votes[i] > most_voted_votes) {
                most_voted_class = i;
                most_voted_votes = votes[i];
            }
        }
        return most_voted_class;
    }
    

int classifier_model_predict_proba(const float *features, int32_t features_length, float *out, int out_length) {

        int32_t _class = -1;

        for (int i=0; i<out_length; i++) {
            out[i] = 0.0f;
        }

        _class = classifier_model_tree_0(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_1(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_2(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_3(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_4(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_5(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_6(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_7(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_8(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_9(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_10(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_11(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_12(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_13(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_14(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_15(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_16(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_17(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_18(features, features_length); out[_class] += 1.0f;
    _class = classifier_model_tree_19(features, features_length); out[_class] += 1.0f;
    
        // compute mean
        for (int i=0; i<out_length; i++) {
            out[i] = out[i] / 20;
        }
        return 0;
    }
    