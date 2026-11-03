
#pragma once

#include <glad/gl.h>

class Ground {
  private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

  public:
    Ground();
    ~Ground();

    Ground(const Ground&) = delete;
    Ground& operator=(const Ground&) = delete;

    void draw() const;
};
