use crate::{App, Result, state::Visual};
use smithay_client_toolkit::shm::{Shm, raw::RawPool};
use std::io::Cursor;
use wayland_client::{
    QueueHandle,
    protocol::{wl_buffer, wl_shm},
};

pub struct Frames {
    pub buffers: [wl_buffer::WlBuffer; 3],
    free: [bool; 3],
    pool: RawPool,
    scene: Scene,
}

impl Frames {
    pub fn new(shm: &Shm, qh: &QueueHandle<App>, width: u32, height: u32) -> Result<Self> {
        let scene = Scene::new(width, height)?;
        let bytes = (width * height * 4) as usize;
        let mut pool = RawPool::new(bytes * 3, shm)?;
        let buffers = std::array::from_fn(|frame| {
            pool.create_buffer(
                (frame * bytes) as i32,
                width as i32,
                height as i32,
                width as i32 * 4,
                wl_shm::Format::Argb8888,
                (),
                qh,
            )
        });
        Ok(Self {
            buffers,
            free: [true; 3],
            pool,
            scene,
        })
    }

    pub fn available(&self) -> bool {
        self.free.contains(&true)
    }

    pub fn release(&mut self, buffer: &wl_buffer::WlBuffer) {
        if let Some(index) = self.buffers.iter().position(|b| b == buffer) {
            self.free[index] = true;
        }
    }

    pub fn draw(&mut self, visual: Visual) -> &wl_buffer::WlBuffer {
        let index = self
            .free
            .iter()
            .position(|&free| free)
            .expect("check available before drawing");
        let rgba = self.scene.draw(visual);
        let bytes = rgba.len();
        let canvas = &mut self.pool.mmap()[index * bytes..(index + 1) * bytes];
        for (src, dst) in rgba.chunks_exact(4).zip(canvas.chunks_exact_mut(4)) {
            let argb = u32::from_be_bytes([src[3], src[0], src[1], src[2]]);
            dst.copy_from_slice(&argb.to_ne_bytes());
        }
        self.free[index] = false;
        &self.buffers[index]
    }
}

impl Drop for Frames {
    fn drop(&mut self) {
        // A resize creates a new memfd. The compositor retains the old backing
        // fd while it is in use; live buffers are never overwritten/recycled.
        for buffer in &self.buffers {
            buffer.destroy();
        }
    }
}

struct Scene {
    #[cfg(test)]
    base: Vec<u8>,
    keyboards: [Vec<u8>; 2],
    clean: [Vec<u8>; 2],
    mouse: Vec<u8>,
    canvas: Vec<u8>,
    width: usize,
    height: usize,
}

const COVER: &[u8] = include_bytes!("../assets/original-cover.png");
const BACKGROUND: &[u8] = include_bytes!("../assets/original-background.png");
const KEY_A: &[u8] = include_bytes!("../assets/original-key-a.png");

fn decode(bytes: &[u8]) -> Result<Vec<u8>> {
    let mut decoder = png::Decoder::new(Cursor::new(bytes));
    decoder.set_transformations(png::Transformations::EXPAND | png::Transformations::STRIP_16);
    let mut reader = decoder.read_info()?;
    let mut rgba = vec![0; reader.output_buffer_size().ok_or("invalid PNG size")?];
    let info = reader.next_frame(&mut rgba)?;
    if info.color_type != png::ColorType::Rgba || (info.width, info.height) != (612, 354) {
        return Err("original artwork must be 612x354 RGBA".into());
    }
    for pixel in rgba.chunks_exact_mut(4) {
        for c in 0..3 {
            pixel[c] = (pixel[c] as u32 * pixel[3] as u32 / 255) as u8;
        }
    }
    Ok(rgba)
}

impl Scene {
    fn new(width: u32, height: u32) -> Result<Self> {
        let cover = decode(COVER)?;
        let background = decode(BACKGROUND)?;
        let key = decode(KEY_A)?;
        let mut pressed = cover.clone();
        // Same raised-paw erase region as the original project's renderer.
        // The input PNGs themselves stay byte-for-byte unchanged.
        for y in 0..354 {
            for x in 0..612 {
                let dx = (x as f32 / 612.0 - 0.700) / 0.080;
                let dy = (y as f32 / 354.0 - 0.515) / 0.170;
                let p = &mut pressed[(y * 612 + x) * 4..][..4];
                if dx * dx + dy * dy < 1.0 {
                    p[0] = p[3];
                    p[1] = p[3];
                    p[2] = p[3];
                }
            }
        }
        for (dst, src) in pressed.chunks_exact_mut(4).zip(key.chunks_exact(4)) {
            over(dst, src);
        }

        let mut mouse = vec![0; cover.len()];
        let mut clean = [cover.clone(), pressed.clone()];
        for y in 170..262 {
            // Follow the original arm's inner edge, excluding the torso.
            let right = (245.0 - (y - 170) as f32 * 1.35).max(165.0) as usize;
            let differs = |x: usize| {
                let i = (y * 612 + x) * 4;
                cover[i + 3] > 0 && (0..4).any(|c| cover[i + c].abs_diff(background[i + c]) > 12)
            };
            let Some(first) = (70..right).find(|&x| differs(x)) else {
                continue;
            };
            let last = (first..right).rfind(|&x| differs(x)).unwrap();
            // Fill interior white-on-white pixels too; a difference mask alone
            // would cut holes through the paw where it overlapped the desk.
            for x in first.saturating_sub(1)..(last + 2).min(right) {
                let i = (y * 612 + x) * 4;
                let src = &cover[i..i + 4];
                if src[3] > 0 {
                    mouse[i..i + 4].copy_from_slice(src);
                    for image in &mut clean {
                        image[i..i + 4].copy_from_slice(&background[i..i + 4]);
                        // The shoulder overlaps the white torso, not empty
                        // desktop; keep that backing opaque when the paw moves.
                        if x >= 180 && (y as f32) < 152.0 + x as f32 * 0.18 {
                            image[i..i + 4].copy_from_slice(&[255; 4]);
                        }
                    }
                }
            }
        }
        let (width, height) = (width as usize, height as usize);
        let resize = |source: &[u8]| {
            let mut dst = vec![0; width * height * 4];
            scale(source, 612, 354, &mut dst, width, height);
            dst
        };
        Ok(Self {
            #[cfg(test)]
            base: resize(&cover),
            keyboards: [resize(&cover), resize(&pressed)],
            clean: [resize(&clean[0]), resize(&clean[1])],
            mouse: resize(&mouse),
            canvas: vec![0; width * height * 4],
            width,
            height,
        })
    }

    fn draw(&mut self, visual: Visual) -> &[u8] {
        let state = (visual.keys & 1) as usize;
        if visual.mouse == [0, 0] && visual.keys & 2 == 0 {
            // Exact original cover at rest, including original pink paw pads.
            self.canvas.copy_from_slice(&self.keyboards[state]);
            return &self.canvas;
        }
        self.canvas.copy_from_slice(&self.clean[state]);
        let sx = self.width as f32 / 612.0;
        let sy = self.height as f32 / 354.0;
        let dx = visual.mouse[0] as f32 * sx;
        let dy = (visual.mouse[1] + if visual.keys & 2 != 0 { 3 } else { 0 }) as f32 * sy;
        let shoulder = 170.0 * sy;
        let wrist = 225.0 * sy;
        // Row-wise affine deformation: the shoulder stays anchored and the
        // original paw/mouse pixels follow the pointer. No redrawn outlines.
        for y in (166.0 * sy) as usize..((294.0 * sy) as usize).min(self.height) {
            let source_y = if y as f32 <= shoulder {
                y as f32
            } else if y as f32 >= wrist + dy {
                y as f32 - dy
            } else {
                (y as f32 + dy * shoulder / (wrist - shoulder)) / (1.0 + dy / (wrist - shoulder))
            };
            let weight = ((source_y - shoulder) / (wrist - shoulder)).clamp(0.0, 1.0);
            let iy = source_y.floor() as usize;
            if iy + 1 >= self.height {
                continue;
            }
            let fy = ((source_y - iy as f32) * 256.0) as u32;
            let offset_x = -dx * weight;
            let ix = offset_x.floor() as i32;
            let fx = ((offset_x - ix as f32) * 256.0) as u32;
            for x in (28.0 * sx) as usize..((272.0 * sx) as usize).min(self.width) {
                let source_x = x as i32 + ix;
                if source_x < 0 || source_x as usize + 1 >= self.width {
                    continue;
                }
                let a = (iy * self.width + source_x as usize) * 4;
                let b = a + self.width * 4;
                let mut sample = [0; 4];
                for (c, value) in sample.iter_mut().enumerate() {
                    let top =
                        self.mouse[a + c] as u32 * (256 - fx) + self.mouse[a + 4 + c] as u32 * fx;
                    let bottom =
                        self.mouse[b + c] as u32 * (256 - fx) + self.mouse[b + 4 + c] as u32 * fx;
                    *value = ((top * (256 - fy) + bottom * fy) >> 16) as u8;
                }
                over(&mut self.canvas[(y * self.width + x) * 4..][..4], &sample);
            }
        }
        &self.canvas
    }
}

fn over(dst: &mut [u8], src: &[u8]) {
    let inverse = 255 - src[3] as u32;
    for c in 0..4 {
        dst[c] = (src[c] as u32 + (dst[c] as u32 * inverse + 127) / 255).min(255) as u8;
    }
}

fn scale(src: &[u8], sw: usize, sh: usize, dst: &mut [u8], dw: usize, dh: usize) {
    for y in 0..dh {
        for x in 0..dw {
            // Average premultiplied colors, preventing transparent-edge halos.
            let x0 = x * sw / dw;
            let x1 = ((x + 1) * sw / dw).max(x0 + 1).min(sw);
            let y0 = y * sh / dh;
            let y1 = ((y + 1) * sh / dh).max(y0 + 1).min(sh);
            let mut sum = [0u64; 4];
            for sy in y0..y1 {
                for sx in x0..x1 {
                    let p = &src[(sy * sw + sx) * 4..][..4];
                    for c in 0..3 {
                        sum[c] += p[c] as u64;
                    }
                    sum[3] += p[3] as u64;
                }
            }
            let n = ((x1 - x0) * (y1 - y0)) as u64;
            let out = &mut dst[(y * dw + x) * 4..][..4];
            for c in 0..3 {
                out[c] = (sum[c] / n) as u8;
            }
            out[3] = (sum[3] / n) as u8;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn idle_is_pixel_identical_to_original_cover() {
        let original = decode(COVER).unwrap();
        let mut scene = Scene::new(612, 354).unwrap();
        assert_eq!(
            scene.draw(Visual {
                keys: 0,
                mouse: [0, 0]
            }),
            original
        );
    }

    #[test]
    fn static_pixels_never_change_between_poses_or_mouse_positions() {
        let mut scene = Scene::new(612, 354).unwrap();
        let base = scene.base.clone();
        for keys in 0..4 {
            for mouse in [[0, 0], [-40, -24], [40, 24]] {
                let image = scene.draw(Visual { keys, mouse });
                for y in 0..354 {
                    for x in 0..612 {
                        let mouse_area = (28..272).contains(&x) && (166..294).contains(&y);
                        let keyboard_paw = (375..500).contains(&x) && (120..298).contains(&y);
                        if !mouse_area && !keyboard_paw {
                            let offset = (y * 612 + x) * 4;
                            assert_eq!(
                                image[offset..offset + 4],
                                base[offset..offset + 4],
                                "changed static pixel ({x},{y})"
                            );
                        }
                    }
                }
            }
        }
    }

    #[test]
    fn mouse_movement_changes_the_paw_and_mouse() {
        let mut scene = Scene::new(306, 177).unwrap();
        let centered = scene
            .draw(Visual {
                keys: 0,
                mouse: [0, 0],
            })
            .to_vec();
        assert_ne!(
            centered,
            scene.draw(Visual {
                keys: 0,
                mouse: [32, 16]
            })
        );
    }

    #[test]
    #[ignore = "measures CPU rendering cost without opening a desktop window"]
    fn render_benchmark() {
        let mut scene = Scene::new(306, 177).unwrap();
        let start = std::time::Instant::now();
        for frame in 0..3600 {
            let visual = Visual {
                keys: (frame % 4) as u8,
                mouse: [frame % 81 - 40, frame % 49 - 24],
            };
            std::hint::black_box(scene.draw(visual));
        }
        let elapsed = start.elapsed();
        eprintln!(
            "3600 frames: {elapsed:?}; {:.3} ms/frame",
            elapsed.as_secs_f64() * 1000.0 / 3600.0
        );
    }

    #[test]
    #[ignore = "writes rendered application previews to target/previews"]
    fn render_previews() {
        std::fs::create_dir_all("target/previews").unwrap();
        let mut scene = Scene::new(612, 354).unwrap();
        for (i, visual) in [
            Visual {
                keys: 0,
                mouse: [0, 0],
            },
            Visual {
                keys: 1,
                mouse: [0, 0],
            },
            Visual {
                keys: 2,
                mouse: [-40, -24],
            },
            Visual {
                keys: 3,
                mouse: [40, 24],
            },
        ]
        .into_iter()
        .enumerate()
        {
            let data = scene.draw(visual);
            let mut rgba = data.to_vec();
            for p in rgba.chunks_exact_mut(4) {
                if p[3] > 0 {
                    for c in 0..3 {
                        p[c] = (p[c] as u32 * 255 / p[3] as u32) as u8;
                    }
                }
            }
            let file = std::fs::File::create(format!("target/previews/state-{i}.png")).unwrap();
            let mut encoder = png::Encoder::new(file, 612, 354);
            encoder.set_color(png::ColorType::Rgba);
            encoder.set_depth(png::BitDepth::Eight);
            encoder
                .write_header()
                .unwrap()
                .write_image_data(&rgba)
                .unwrap();
        }
    }
}
