import { BrowserRouter, Navigate, Route, Routes } from 'react-router-dom';
import { ProveedorSesion, useSesion } from './auth/sesion';
import { Layout } from './componentes/Layout';
import { Login } from './paginas/Login';
import { ListadoEspecies } from './paginas/ListadoEspecies';
import { FormularioEspecie } from './paginas/FormularioEspecie';
import { Postulaciones } from './paginas/Postulaciones';
import { Avistamientos } from './paginas/Avistamientos';
import { Usuarios } from './paginas/Usuarios';

function Privadas() {
  const { usuario } = useSesion();
  if (!usuario) return <Navigate to="/login" replace />;

  return (
    <Layout>
      <Routes>
        <Route path="/especies" element={<ListadoEspecies />} />
        <Route path="/especies/nueva" element={<FormularioEspecie />} />
        <Route path="/especies/:id" element={<FormularioEspecie />} />
        <Route path="/postulaciones" element={<Postulaciones />} />
        <Route path="/avistamientos" element={<Avistamientos />} />
        <Route path="/usuarios" element={<Usuarios />} />
        <Route path="*" element={<Navigate to="/especies" replace />} />
      </Routes>
    </Layout>
  );
}

export function App() {
  return (
    <BrowserRouter basename={import.meta.env.BASE_URL}>
      <ProveedorSesion>
        <Routes>
          <Route path="/login" element={<Login />} />
          <Route path="*" element={<Privadas />} />
        </Routes>
      </ProveedorSesion>
    </BrowserRouter>
  );
}
