import type { ReactNode } from 'react';
import { NavLink } from 'react-router-dom';
import { useSesion } from '../auth/sesion';

export function Layout({ children }: { children: ReactNode }) {
  const { usuario, esGlobal, salir } = useSesion();

  return (
    <div className="layout">
      <header className="barra">
        <span className="marca">Curaduría · Biodiversidad de Chiloé</span>
        <nav>
          <NavLink to="/especies">Especies</NavLink>
          <NavLink to="/avistamientos">Avistamientos</NavLink>
          {/* La bandeja de postulaciones solo la resuelve un admin: mostrarla a
              un curador sería ofrecerle una pantalla que devuelve 403. */}
          {usuario?.role === 'admin' && <NavLink to="/postulaciones">Postulaciones</NavLink>}
          {/* Igual que postulaciones: el listado de usuarios lo responde la API
              solo a un admin. */}
          {usuario?.role === 'admin' && <NavLink to="/usuarios">Usuarios</NavLink>}
        </nav>
        <span className="sesion">
          {usuario?.name} · {esGlobal ? usuario?.role : 'curador'}
          <button type="button" onClick={salir}>
            Salir
          </button>
        </span>
      </header>
      <main>{children}</main>
    </div>
  );
}
