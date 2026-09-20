# <img src="https://github.com/Elchi-2023/witches-akashi-party/blob/tea/data/icon/wap_akashi.ico" width="24" height="24"> Witches Akashi Party

**An unofficial & community-driven fork of Akashi — A Qt C++ server for Attorney Online 2**

![GitHub License](https://img.shields.io/github/license/Elchi-2023/witches-akashi-party)
![GitHub repo size](https://img.shields.io/github/repo-size/Elchi-2023/witches-akashi-party)
![GitHub commit activity](https://img.shields.io/github/commit-activity/t/Elchi-2023/witches-akashi-party/tea)
![GitHub Actions](https://img.shields.io/github/actions/workflow/status/Elchi-2023/witches-akashi-party/test.yml)
![Last commit](https://img.shields.io/github/last-commit/Elchi-2023/witches-akashi-party/tea)

> **Disclaimer**: Witches Akashi Party is an unofficial, community-driven fork of Akashi. It is not officially affiliated with or endorsed by the Attorney Online project. We are grateful for the original work by the Attorney Online team and all contributors, and we continue to build upon it under the terms of the license.

## Q&A

**Q: Where can I download this?**  
**A**: We don't provide pre-built releases yet. Please compile from source.

**Q: How do I get support?**  
**A**: This is an unofficial software fork. For general Attorney Online questions, join the [official Discord](https://discord.gg/wWvQ3pw).  
For fork-specific issues, feel free to open an Issue here.

**Q: Can I build with Qt5?**  
**A**: Yes, Qt5 still works for most features. However, Qt6 is recommended if possible for better long-term compatibility and performance.

**Q: Can I fork or modify this project?**  
**A**: Yes! Just follow the [AGPL-3.0 license](LICENSE.md) and give proper credit to the original contributors.

**Q: Why is this fork different from official Akashi?**  
**A**: We added custom features, optimizations, and quality-of-life changes. It's a community-driven project, so things evolve faster but may also have more experimental bugs.

## Build Instructions

This branch (**tea**) currently targets **Qt 5**.

**Requirements:**

- Qt 5.x (with Qt WebSockets module)
- C++17 compatible compiler
- CMake

Qt 6 support is planned for future development, but is not required for this branch.

See the [original Akashi Build Guide](https://github.com/AttorneyOnline/akashi/wiki/Building-Akashi) for general instructions. Some adjustments may be needed for Qt 5.

**Note for older systems (Windows 7, etc.):** Qt 5 is still the most compatible choice for now.

## Docker

```bash
docker compose up -d
```

## Credits

**Big thanks to:**

- ![Elchi-2023Icon](https://avatars.githubusercontent.com/u/136360907?s=18&v=4) **[Elchi-2023](https://github.com/Elchi-2023)** — Creator of Witches Akashi Party
- ![Ganty1999Icon](https://avatars.githubusercontent.com/u/48977410?s=18&v=4) **[Ganty1999](https://github.com/Ganty1999)** — Collaborator & maintainer
- ![SyntaxNyahIcon](https://avatars.githubusercontent.com/u/235356625?s=18&v=4) **[SyntaxNyah](https://github.com/SyntaxNyah)** — Major improvements and collaborator

**Upstream:**

- ![AttorneyOnline](https://avatars.githubusercontent.com/u/25104739?s=18&v=4) **[AttorneyOnline/akashi](https://github.com/AttorneyOnline/akashi)** — Original software this fork is based on

**Note**: This fork has evolved quite far from the original Akashi with custom features, optimizations, and more. It's a community-driven project!
