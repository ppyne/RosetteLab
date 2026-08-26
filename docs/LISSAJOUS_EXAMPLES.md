# Lissajous example grid

RosetteLab uses one general Lissajous definition:

\[
x(t)=A_x\sin(f_xt+\varphi_x),\qquad
y(t)=A_y\sin(f_yt+\varphi_y)
\]

The 56 basic graphs in Figure 1 of Wang, Zhang, and You, *Design rules for
dense and rapid Lissajous scanning*, are all produced by this definition. No
additional curve mode is required.

Use equal amplitudes for the proportions shown in the paper, for example
`Amplitude X = 80` and `Amplitude Y = 80`. Set `Phase X = 90°`, then select the
frequency ratio and `Phase Y` from the table below.

The paper labels its columns with an auxiliary value `k` and defines:

\[
\varphi_y=90^\circ+\frac{45^\circ k}{n_x}
\]

Here `Frequency X = nx` and `Frequency Y = ny`. The paper's `k` is not a
separate RosetteLab parameter and must not be confused with the polar-rose
parameter of the same name.

| Frequency X:Y | k=0 | k=1 | k=2 | k=3 | k=4 | k=5 | k=6 | k=7 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1:1 | 90° | 135° | 180° | 225° | 270° | 315° | 360° | 405° |
| 2:1 | 90° | 112.5° | 135° | 157.5° | 180° | 202.5° | 225° | 247.5° |
| 3:1 | 90° | 105° | 120° | 135° | 150° | 165° | 180° | 195° |
| 3:2 | 90° | 105° | 120° | 135° | 150° | 165° | 180° | 195° |
| 4:3 | 90° | 101.25° | 112.5° | 123.75° | 135° | 146.25° | 157.5° | 168.75° |
| 5:3 | 90° | 99° | 108° | 117° | 126° | 135° | 144° | 153° |
| 5:4 | 90° | 99° | 108° | 117° | 126° | 135° | 144° | 153° |

Angles greater than or equal to 360° are intentionally left in their derived
form; RosetteLab may normalize them without changing the curve. Decimal values
between the eight documented columns are valid and produce continuous
intermediate variations.

Source: [Design rules for dense and rapid Lissajous scanning](https://www.nature.com/articles/s41378-020-00211-4)
